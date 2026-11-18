// Note 4 e-paper UI: screen model, skin interface and drawing helpers.
//
// CONTRACT FILE. core.yaml, the framework (note4_ui.cpp, canvas.cpp) and every
// skin are written against this header. Change it only together with all of
// them.
//
// Colour convention (read by the zectrix_epd driver):
//   ink(g)  = Color(g, g, g, 0)    text, lines, solid shapes. On a 1-bit refresh
//                                  thresholded (crisp).
//   tone(g) = Color(g, g, g, 255)  flat grey fills. On a 16-grey refresh drawn as
//                                  grey; on a 1-bit refresh rendered as an 8x8
//                                  Bayer raster.
// Anti-aliased font edges blend the w channel too, so text over a tone keeps
// mixed edges. Always pass the surface colour as `bg` when printing over a fill.
//
// Live regions (anything refreshed partially: clock, timer, waveform, typing)
// must sit on pure white or pure black, never on a grey fill: partial refresh
// is 1-bit, and a grey there turns into dots.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"

#include "strings.h"

namespace esphome {
namespace note4_ui {

using display::Display;
using font::Font;

static const int W = 400;
static const int H = 300;

inline Color ink(uint8_t g = 0) { return Color(g, g, g, 0); }
inline Color tone(uint8_t g) { return Color(g, g, g, 255); }
inline Color paper() { return Color(255, 255, 255, 0); }

struct Rect {
  int x, y, w, h;
};

enum class Screen : uint8_t {
  IDLE,      // page of the day: clock, date, weather, day timeline, cards
  LISTEN,    // woken, recording: waveform + elapsed seconds
  THINK,     // STT done: heard question + pipeline steps
  ANSWER,    // question + answer, typed word by word while TTS plays
  WEATHER,   // tomorrow's forecast card
  TIMER,     // running timer
  DOORBELL,  // camera snapshot + caption + actions
};

enum class Refresh : uint8_t { FULL_MONO, FULL_GRAY, PARTIAL };

// The "Now" card (what matters right now). HA picks one item by priority and
// sends it.
enum class NowLevel : uint8_t {
  BRIEF = 0,  // nothing urgent: one-sentence day brief (AI Task)
  TODO = 1,   // window open, MeteoAlarm, plant, vacuum, school grade ...
  ALARM = 2,  // smoke, flood: drawn in negative, the loudest thing on screen
};

struct Model {
  // clock / status
  int hour = 0, minute = 0, second = 0;
  std::string weekday;       // "Tuesday", in the active language (core.yaml)
  std::string date;          // "17 November"
  std::string room = "Kitchen";
  int battery = -1;          // %, -1 unknown
  bool charging = false;
  bool wifi = false;
  bool ha = false;
  std::string wake_word = "Alexa";

  // weather now (idle)
  float temp_now = NAN, temp_hi = NAN, temp_lo = NAN;
  std::string condition;     // "Fog, then sun"
  std::string condition_icon;  // HA condition: "fog", "rainy", "sunny", ...

  // day timeline + next event
  struct Event { float start_h, end_h; std::string title; };
  std::vector<Event> events;           // today, hours as float (9.5 = 9:30)
  std::string next_time, next_title, next_in;  // "17:00", "Parcel pickup", "in 2 h 18 min" (all from HA)

  // the "Now" card
  NowLevel now_level = NowLevel::BRIEF;
  std::string now_title;   // "Window open"
  std::string now_body;    // "Bathroom · 12 min"
  std::string now_detail;  // "heating paused"
  std::string now_icon;    // "window", "wind", "plant", "vacuum", "school", "flood", "smoke", "brief"

  // conversation
  std::string question;
  std::string answer;
  int answer_words_shown = -1;   // -1 = all; >=0 while typing
  int pipeline_step = 0;         // THINK: 0 stt, 1 asking HA, 2 composing
  std::array<float, 92> mic_history{};  // 0..1, newest last
  int listen_seconds = 0;
  std::vector<std::pair<std::string, std::string>> facts;  // answer key facts ("15:00", "rain starts")

  // tomorrow's forecast
  std::string fc_day, fc_date;            // "Wednesday", "18 November" (from HA)
  float fc_hi = NAN, fc_lo = NAN;
  std::string fc_summary;                 // "Rain from 15:00"
  std::string fc_detail;                  // "4 mm in total"
  std::array<float, 19> fc_temp{};        // hours 6..24
  std::array<float, 19> fc_rain{};        // mm per hour, hours 6..24
  float fc_rain_start_h = NAN;
  std::string fc_wind, fc_humidity, fc_sunset, fc_sunrise;

  // timer
  bool timer_active = false;
  std::string timer_name;   // "Pasta"
  int timer_remaining = 0;  // s
  int timer_total = 0;      // s
  std::string timer_start, timer_end;  // "17:35", "17:45"

  // doorbell
  // bell_place: Strings::default_place until HA names one; caption from AI Task.
  std::string bell_place = "Gate", bell_time, bell_caption;
  bool bell_image_ready = false;
  display::BaseImage *bell_image = nullptr;  // online_image (greyscale), set from YAML

  // On-screen language (strings.h). Skins draw every phrase from here:
  // `const Strings &T = *m.tr;`. Set with Note4UI::set_language().
  const Strings *tr = &strings_en();
};

// Drawing helpers (canvas.cpp). Angles in radians, 0 = 3 o'clock, clockwise
// (screen coordinates). TOP = -pi/2.
static const float TOP = -1.5707963f;
enum class Align : uint8_t { LEFT, CENTER, RIGHT };

// Text at a BASELINE, like the mockups. `ls` = extra letter spacing in px
// (drawn glyph by glyph when non-zero).
void text(Display &d, Font *f, int x, int y, Color c, const std::string &s, Align a = Align::LEFT,
          Color bg = paper(), float ls = 0);
int measure(Font *f, const std::string &s, float ls = 0);
std::vector<std::string> wrap(Font *f, const std::string &s, int max_w);
// Digits in fixed-width cells so a ticking clock does not jitter.
void fixed_digits(Display &d, Font *f, int cx, int y, Color c, const std::string &s, Color bg = paper());

void fill_rect(Display &d, int x, int y, int w, int h, Color c);
void stroke_rect(Display &d, int x, int y, int w, int h, Color c, int t = 1);
void fill_round(Display &d, int x, int y, int w, int h, int r, Color c);
void stroke_round(Display &d, int x, int y, int w, int h, int r, Color c, int t = 1);
void hline(Display &d, int x1, int x2, int y, Color c, int t = 1);
void vline(Display &d, int x, int y1, int y2, Color c, int t = 1);
void dotted_hline(Display &d, int x1, int x2, int y, Color c, int on = 2, int off = 2);
void dotted_vline(Display &d, int x, int y1, int y2, Color c, int on = 2, int off = 2);
void thick_line(Display &d, float x1, float y1, float x2, float y2, float w, Color c, bool round_caps = false);
void fill_circle(Display &d, float cx, float cy, float r, Color c);
void ring(Display &d, float cx, float cy, float r, float t, Color c);  // centred on r
void arc(Display &d, float cx, float cy, float r, float t, float a0, float a1, Color c, bool round_caps = false);
void wedge(Display &d, float cx, float cy, float r, float a0, float a1, Color c);
void fill_ellipse(Display &d, float cx, float cy, float rx, float ry, float rot, Color c);
void fill_poly(Display &d, const std::vector<std::pair<float, float>> &pts, Color c);
void polyline(Display &d, const std::vector<std::pair<float, float>> &pts, float w, Color c);
// rows of '0'/'1', scaled by s (pixel-art icons).
void bitmap(Display &d, const std::vector<const char *> &rows, int x, int y, int s, Color c);
// Glyph-style button hints: ▲ ▼ ● drawn as shapes.
enum class Sym : uint8_t { UP, DOWN, DOT };
void sym(Display &d, Sym s, int x, int y, int size, Color c);

std::string mmss(int seconds);   // "07:42"
std::string hhmm(int h, int m);  // "17:45"

// ---------------------------------------------------------------------------
class Skin {
 public:
  virtual ~Skin() = default;
  virtual const char *id() const = 0;  // "paper", "swiss", ...
  virtual void draw(Display &d, Screen s, const Model &m) = 0;
  // Regions that change between full refreshes on this screen (clock digits,
  // timer, waveform, current answer line). Must lie on white or black.
  virtual std::vector<Rect> live_regions(Screen s, const Model &m) const = 0;
  // May this screen use a 16-grey full refresh? Screens with live regions
  // cannot (the driver forbids partial refresh after a grey one).
  virtual bool gray_allowed(Screen s) const;
};

class Note4UI : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_display(Display *d) { display_ = d; }
  // Driver hook, set from YAML: performs the refresh the UI decided on.
  void set_refresh_handler(std::function<void(Refresh, const std::vector<Rect> &)> h) { refresh_ = std::move(h); }

  void add_skin(Skin *s);
  bool set_skin(const std::string &id);  // false if not compiled in
  const std::string &skin_id() const;
  std::vector<std::string> skin_ids() const;

  void show(Screen s);       // full refresh
  Screen screen() const { return screen_; }
  Model &model() { return model_; }
  // Something in the model changed. `live` = only live regions changed
  // (partial); otherwise the current screen is redrawn with a full refresh.
  void changed(bool live);
  void set_gray_enabled(bool on) { gray_enabled_ = on; }
  // On-screen language: "en" (default), "pl", "de", "nl", "fr", "es". False
  // (and the current one kept) for an unknown code. Call before the model is
  // filled (core.yaml on_boot).
  bool set_language(const std::string &code);
  const Strings &strings() const { return *model_.tr; }

  // Called from the display lambda.
  void draw(Display &d);

 protected:
  void request_(Refresh r, const std::vector<Rect> &rects);
  Display *display_{nullptr};
  std::function<void(Refresh, const std::vector<Rect> &)> refresh_;
  std::vector<Skin *> skins_;
  Skin *skin_{nullptr};
  Screen screen_{Screen::IDLE};
  Model model_;
  bool gray_enabled_{true};
  bool dirty_full_{false}, dirty_live_{false};
  uint32_t last_request_ms_{0};
};

}  // namespace note4_ui
}  // namespace esphome
