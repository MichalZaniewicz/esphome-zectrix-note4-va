// Shared helpers for the paper, swiss, instrument and tiles skins:
// Polish text utilities, forecast maths, answer typing layout and a few
// pictograms that several mockups share (cloud, sun, keycap).
#pragma once

#include <string>
#include <vector>

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {
namespace uu {

// Grey text on white (the mockups' ttext()). Ink is thresholded at grey 8 of
// 15 on a 1-bit refresh, so lighter greys would vanish: capped at 120, which
// stays black on 1-bit and reads as mid grey on a 16-grey refresh.
inline Color grey(uint8_t g) { return ink(g < 120 ? g : 120); }

// ---- text ----
std::string upper(const std::string &s);        // ASCII, Latin-1, Latin Extended-A (Polish)
std::string cap_first(const std::string &s);    // "środa" -> "Środa"
std::string lower_first(const std::string &s);  // "Deszcz od 15:00" -> "deszcz od 15:00"
std::string weekday_short(const std::string &weekday);  // "wtorek" -> "wt"
std::string ddmm(const std::string &date);               // "17 listopada" -> "17.11"
std::string first_clause(const std::string &s);          // "Mgła, potem słońce" -> "Mgła"
// Cut `s` with a trailing ellipsis so it fits in max_w.
std::string fit(Font *f, const std::string &s, int max_w, float ls = 0);
// wrap() limited to max_lines; the last kept line gets an ellipsis if text was cut.
std::vector<std::string> wrap_lines(Font *f, const std::string &s, int max_w, int max_lines);
std::vector<std::string> split_words(const std::string &s);

// ---- numbers ----
std::string deg(float v);          // "4°", "-3°", "–" when NAN
std::string signed_deg(float v);   // "+4°", "-3°"
std::string signed_int(int v);     // "+4", "-3", "0" -> "+0"
std::string fmt1(float v);         // "4.0"
std::string dur_label(int s);      // 300 -> "5 min", 90 -> "1:30"
bool parse_hhmm(const std::string &s, int &h, int &m);

// ---- model ----
float now_hours(const Model &m);              // clamped to the 6..22 day axis
float timer_frac(const Model &m);             // remaining / total, 0..1
float fc_temp_at(const Model &m, float h);    // smooth (cosine) between hourly values, h in 6..24
void fc_range(const Model &m, float &lo, float &hi);  // min/max of the hourly temperatures
int fc_peak_index(const Model &m);           // index (hour - 6) of the warmest hour
float fc_rain_max(const Model &m);
float fc_rain_sum(const Model &m);
bool fc_has_rain(const Model &m);
std::string brief_text(const Model &m);       // NowLevel::BRIEF sentence: now_body, else now_title
std::string now_kind(const std::string &icon);  // "window" -> "Okno", "wind" -> "Ostrzeżenie", ...
// "T-02:18" style countdown to next_time, or "" when it cannot be computed.
std::string countdown(const Model &m);

// ---- answer typing ----
struct AnswerLayout {
  std::vector<std::string> lines;  // displayed lines (only the words shown so far)
  int cursor_line = 0;             // index into `lines` of the line being typed
  int cursor_w = 0;                // width of that line's text
  bool typing = false;             // answer_words_shown hides some words
  bool scrolled = false;           // more lines than fit; window follows the cursor
  int max_lines = 0;
};
// Lays out the whole answer first (lines never reflow while typing), then
// keeps the shown words. A complete answer that does not fit ends with "…".
AnswerLayout layout_answer(Font *f, const Model &m, int max_w, int max_lines);
// Live rects while typing: the current line plus the one above (the cursor
// block leaves it when a line wraps). Line i occupies
// [y0 + i*pitch - up, y0 + i*pitch - up + h). Whole block when scrolled.
std::vector<Rect> typing_rects(const AnswerLayout &L, int x, int w, int y0, int pitch, int up, int h);

// ---- pictograms ----
// Mockup cloud: three circles and a base, vertical tone gradient top->bottom,
// optional 2 px ink outline.
void cloud(Display &d, float cx, float cy, float s, uint8_t top, uint8_t bottom, bool outline,
           Color outline_c = ink(0));
// Radial-gradient sun disc (tone), optionally with eight ink rays and an ink rim.
void sun(Display &d, float cx, float cy, float r, uint8_t inner, uint8_t outer, bool rays, Color line_c = ink(0));
// Moon crescent (tone fill + optional ink rim).
void moon(Display &d, float cx, float cy, float r, uint8_t g, bool rim);
// Bolt for thunderstorms.
void bolt(Display &d, float x, float y, float s, Color c);
// Keycap: rounded 16 px square with a ▲ ▼ ● symbol and a label (mockup keycap()).
void keycap(Display &d, Font *f, Sym s, const std::string &label, int x, int y, Align a, Color c = ink(0),
            Color bg = paper());
// Hint: bare symbol (size 8) and label (mockup hint()).
void hint(Display &d, Font *f, Sym s, const std::string &label, int x, int y, Align a, Color c = ink(0),
          Color bg = paper());
// HA weather condition pictogram built from cloud/sun/moon. (cx, cy, s) are
// the cloud's centre and scale, as in the mockups' cloud(cx, cy, s).
struct IconStyle {
  bool outline = true;          // 2 px ink rim around the cloud
  uint8_t cloud_top = 252, cloud_bottom = 176;
  uint8_t sun_inner = 245, sun_outer = 150;
  bool sun_rays = true;         // rays + rim (paper); false = soft disc (tiles)
  Color line = ink(0);          // rain strokes, snow dots, wind lines
  float line_w = 2.0f;
  bool round = false;           // round caps on rain strokes
};
void weather_icon(Display &d, const std::string &cond, float cx, float cy, float s, const IconStyle &st);
// Check mark stroke.
void check(Display &d, float x, float y, float s, float w, Color c);

}  // namespace uu
}  // namespace note4_ui
}  // namespace esphome
