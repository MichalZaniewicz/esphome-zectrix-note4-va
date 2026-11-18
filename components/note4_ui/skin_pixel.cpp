#include "skin_pixel.h"
#include "skin_efgh.h"

namespace esphome {
namespace note4_ui {

using namespace efgh;

namespace {

using BM = std::vector<const char *>;

// Pixel icons (from the mockup, plus a few for the "Now" card).
const BM BM_CLOUD = {"0000011110000000", "0000111111110000", "0001111111111000", "0111111111111100", "1111111111111110",
                     "1111111111111111", "1111111111111111", "0111111111111110", "0011111111111100"};
const BM BM_CLOUD_IN = {"0000000000000000", "0000000000000000", "0000011100000000", "0001100000000000",
                        "0010000000000000"};
const BM BM_SUN = {"000010000", "010000010", "000111000", "001111100", "101111101",
                   "001111100", "000111000", "010000010", "000010000"};
const BM BM_MOON = {"000111000", "001100000", "011000000", "011000000", "011000000",
                    "011000000", "001100000", "000111000"};
const BM BM_RAIN = {"0100010001000100", "1000100010001000"};
const BM BM_SNOW = {"0100010001000100", "0000000000000000", "0001000100010001"};
const BM BM_FOG = {"1111111111111111", "0000000000000000", "0011111111111100"};
const BM BM_UMBRELLA = {"000111000", "011111110", "111111111", "100010001", "000010000",
                        "000010000", "000010000", "010010000", "001100000"};
const BM BM_DROP = {"0001000", "0001000", "0010100", "0010100", "0100010", "1000001", "1000001", "0100010", "0011100"};
const BM BM_THERMO = {"01110", "01010", "01010", "01010", "01110", "11111", "11111", "11111", "01110"};
const BM BM_ARROW = {"1111111", "0111110", "0011100", "0001000"};
const BM BM_EXCL = {"111", "111", "111", "111", "111", "111", "000", "111", "111"};
const BM BM_WINDOW = {"111111111", "100010001", "100010001", "100010001", "111111111",
                      "100010001", "100010001", "100010001", "111111111"};
const BM BM_WIND = {"000001100", "000000010", "111111110", "000000000", "111111111",
                    "000000000", "111111100", "000000010", "000001100"};
const BM BM_PLANT = {"000000110", "011000111", "011101110", "001111100", "000010000",
                     "000010000", "011111110", "001111100", "001111100"};
const BM BM_VACUUM = {"001111100", "010000010", "100011001", "100011001", "100000001",
                      "100000001", "100000001", "010000010", "001111100"};
const BM BM_BOOK = {"011101110", "100010001", "100010001", "100010001", "100010001", "100010001", "111111111"};
const BM BM_SMOKE = {"000110000", "001000000", "000100000", "000010000", "000100000",
                     "011111110", "111111111", "111111111", "011111110"};
const BM BM_BELL = {"000010000", "001111100", "011111110", "011111110", "011111110",
                    "011111110", "111111111", "000000000", "000111000"};
const BM BM_CLOCK = {"001111100", "010010010", "100010001", "100010001", "100011101",
                     "100000001", "100000001", "010000010", "001111100"};
const BM BM_BOX = {"1111111", "1000001", "1000001", "1000001", "1000001", "1000001", "1111111"};
const BM BM_BOX_OK = {"1111111", "1000001", "1000011", "1010101", "1001001", "1000001", "1111111"};
const BM BM_PLAY = {"1000000", "1100000", "1110000", "1111000", "1110000", "1100000", "1000000"};

const BM &now_bitmap(const std::string &icon) {
  if (icon == "window")
    return BM_WINDOW;
  if (icon == "wind")
    return BM_WIND;
  if (icon == "plant")
    return BM_PLANT;
  if (icon == "vacuum")
    return BM_VACUUM;
  if (icon == "school")
    return BM_BOOK;
  if (icon == "flood")
    return BM_DROP;
  if (icon == "smoke")
    return BM_SMOKE;
  if (icon == "brief")
    return BM_SUN;
  return BM_EXCL;
}

int bm_w(const BM &b) { return b.empty() ? 0 : (int) strlen(b[0]); }

// Weather now, drawn in the box x..x+88, y..y+46.
void weather_icon(Display &d, const std::string &c, int x, int y) {
  if (c == "sunny") {
    bitmap(d, BM_SUN, x + 24, y, 5, ink());
  } else if (c == "clear-night") {
    bitmap(d, BM_MOON, x + 30, y + 2, 5, ink());
  } else if (c == "windy" || c == "windy-variant") {
    bitmap(d, BM_WIND, x + 24, y, 5, ink());
  } else if (c == "partlycloudy" || c.empty()) {
    bitmap(d, BM_SUN, x + 34, y, 3, ink());
    bitmap(d, BM_CLOUD, x, y + 16, 3, ink());
    bitmap(d, BM_CLOUD_IN, x, y + 16, 3, paper());
  } else {
    bitmap(d, BM_CLOUD, x + 4, y + 4, 3, ink());
    bitmap(d, BM_CLOUD_IN, x + 4, y + 4, 3, paper());
    if (c == "rainy" || c == "pouring" || c == "lightning-rainy" || c == "snowy-rainy" || c == "hail")
      bitmap(d, BM_RAIN, x + 6, y + 36, 3, ink());
    else if (c == "snowy")
      bitmap(d, BM_SNOW, x + 6, y + 34, 3, ink());
    else if (c == "fog")
      bitmap(d, BM_FOG, x + 4, y + 34, 3, ink());
  }
}

std::string pad2(int v) {
  char b[8];
  snprintf(b, sizeof(b), "%02d", v);
  return b;
}

}  // namespace

void PixelSkin::draw(Display &d, Screen s, const Model &m) {
  switch (s) {
    case Screen::IDLE:
      this->idle_(d, m);
      break;
    case Screen::LISTEN:
      this->listen_(d, m);
      break;
    case Screen::THINK:
      this->think_(d, m);
      break;
    case Screen::ANSWER:
      this->answer_(d, m);
      break;
    case Screen::WEATHER:
      this->weather_(d, m);
      break;
    case Screen::TIMER:
      this->timer_(d, m);
      break;
    case Screen::DOORBELL:
      this->doorbell_(d, m);
      break;
  }
}

std::vector<Rect> PixelSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{10, 24, 256, 62}};
      if (m.timer_active)
        r.push_back({266, 138, 120, 156});
      return r;
    }
    case Screen::LISTEN:
      return {{300, 0, 100, 20}, {14, 40, 372, 134}};
    case Screen::THINK:
      return {{20, 126, 360, 112}};
    case Screen::ANSWER: {
      AnswerLayout a = layout_answer(f_vt_, m.answer, 344, this->answer_lines_(m), m.answer_words_shown);
      int asc = f_vt_->get_baseline();
      return typing_regions(a, 18, 364, 110, 24, asc + 1, f_vt_->get_height() - asc);
    }
    case Screen::TIMER:
      return {{50, 70, 300, 68}, {60, 154, 320, 32}};
    default:
      return {};
  }
}

int PixelSkin::answer_lines_(const Model &m) const { return m.facts.empty() ? 8 : 6; }

void PixelSkin::titlebar_(Display &d, const std::string &l, const std::string &r) {
  fill_rect(d, 0, 0, W, 20, ink());
  int rw = r.empty() ? 0 : measure(f_s_, r) + 16;
  text(d, f_s_, 8, 14, paper(), fit(f_s_, l, 384 - rw), Align::LEFT, ink());
  if (!r.empty())
    text(d, f_s_, 392, 14, paper(), r, Align::RIGHT, ink());
}

// White window with a double frame and an optional title cut into the top edge.
void PixelSkin::win_(Display &d, int x, int y, int w, int h, const std::string &title) {
  fill_rect(d, x, y, w, h, paper());
  stroke_rect(d, x, y, w, h, ink(), 2);
  stroke_rect(d, x + 4, y + 4, w - 8, h - 8, ink(), 1);
  if (!title.empty()) {
    int tw = measure(f_s_, title) + 8;
    fill_rect(d, x + 8, y - 1, tw, 10, paper());
    text(d, f_s_, x + 12, y + 8, ink(), title);
  }
}

// Round console button: 'A', 'B' or 'v' (the down arrow).
void PixelSkin::button_(Display &d, int x, int cy, char key, int r, const std::string &label, int label_dx,
                         int label_dy) {
  fill_circle(d, x, cy, r, ink());
  if (key == 'v')
    bitmap(d, BM_ARROW, x - 7, cy - 4, 2, paper());
  else
    text(d, f_s_, x, cy + 4, paper(), std::string(1, key), Align::CENTER, ink());
  text(d, f_vt_, x + label_dx, cy + label_dy, ink(), label);
}

// ------------------------------------------------------------------ IDLE
void PixelSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  std::string right = m.battery >= 0 ? std::string(m.charging ? "BAT+ " : "BAT ") + std::to_string(m.battery) + "%" : "";
  if (!m.wifi)
    right = upper(T.no_wifi) + (right.empty() ? "" : "  " + right);
  this->titlebar_(d, upper(m.room), right);

  text(d, f_xl_, 14, 80, ink(), hhmm(m.hour, m.minute));
  text(d, f_vt_, 16, 106, ink(), fit(f_vt_, m.weekday + ", " + m.date, 270));
  weather_icon(d, m.condition_icon, 296, 28);
  text(d, f_m_, 384, 104, ink(), temp_str(m.temp_now, true), Align::RIGHT);
  text(d, f_vt_, 384, 124, ink(), fit(f_vt_, lower(first_part(m.condition)), 120), Align::RIGHT);

  this->win_(d, 14, 138, 118, 156, upper(T.next));
  if (m.next_time.empty() && m.next_title.empty()) {
    auto l = wrap_n(f_vt_, lower(T.nothing_more_today), 100, 3);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_vt_, 26, 180 + (int) i * 20, ink(), fit(f_vt_, l[i], 100));
  } else {
    text(d, f_m_, 26, 176, ink(), fit(f_m_, m.next_time, 96));
    auto l = wrap_n(f_vt_, m.next_title, 96, 3);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_vt_, 26, 204 + (int) i * 20, ink(), l[i]);
    text(d, f_vt_, 26, 276, ink(105), fit(f_vt_, m.next_in, 96));
  }

  this->now_win_(d, m);

  if (m.timer_active) {
    this->win_(d, 266, 138, 120, 156, upper(T.timer));
    text(d, f_m_, 278, 180, ink(), mmss(m.timer_remaining));
    text(d, f_s_, 278, 200, ink(), fit(f_s_, upper(m.timer_name), 100));
    int on = (int) ceilf(timer_frac(m) * 10 - 0.001f);
    for (int i = 0; i < 10; i++) {
      int x = 278 + i * 10;
      if (i < on)
        fill_rect(d, x, 214, 8, 12, ink());
      else
        checker(d, x, 214, 8, 12);
    }
    text(d, f_vt_, 278, 256, ink(), subst(T.of_total, mmss(m.timer_total)));
  } else {
    // no timer: a status window, the battery as a health bar
    this->win_(d, 266, 138, 120, 156, upper(T.status));
    text(d, f_s_, 278, 170, ink(), fit(f_s_, upper(T.battery), 100));
    int on = m.battery < 0 ? 0 : (m.battery + 5) / 10;
    for (int i = 0; i < 10; i++) {
      int x = 278 + i * 10;
      if (i < on)
        fill_rect(d, x, 178, 8, 12, ink());
      else
        checker(d, x, 178, 8, 12);
    }
    text(d, f_vt_, 278, 214, ink(), m.battery >= 0 ? std::to_string(m.battery) + "%" + (m.charging ? " +" : "") : "--%");
    text(d, f_vt_, 278, 242, ink(), m.wifi ? std::string("wifi: ok") : "wifi: " + std::string(T.missing));
    text(d, f_vt_, 278, 266, ink(), m.ha ? std::string("HA: ok") : "HA: " + std::string(T.missing));
  }
}

// The "Now" window in the middle (140,138 118x156).
void PixelSkin::now_win_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const int x = 140, y = 138, w = 118, h = 156, cx = x + w / 2 + 1;
  if (m.now_level == NowLevel::ALARM) {
    this->win_(d, x, y, w, h, upper(T.attention));
    fill_rect(d, x + 8, y + 12, w - 16, h - 20, ink());
    bitmap(d, BM_EXCL, cx - 6, y + 22, 4, paper());
    auto t = wrap_n(f_s_, upper(m.now_title), 98, 2);
    for (size_t i = 0; i < t.size(); i++)
      text(d, f_s_, cx, y + 84 + (int) i * 10, paper(), t[i], Align::CENTER, ink());
    int yb = y + 84 + (int) (t.size() - 1) * 10 + 24;
    text(d, f_vt_, cx, yb, paper(), fit(f_vt_, m.now_body, 98), Align::CENTER, ink());
    if (yb + 20 < y + h - 8)
      text(d, f_vt_, cx, yb + 20, paper(), fit(f_vt_, m.now_detail, 98), Align::CENTER, ink());
    return;
  }
  if (m.now_level == NowLevel::BRIEF) {
    this->win_(d, x, y, w, h, upper(T.today));
    auto l = wrap_n(f_vt_, brief_text(m), 100, 6);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_vt_, x + 10, y + 30 + (int) i * 21, ink(), l[i]);
    return;
  }
  this->win_(d, x, y, w, h, upper(T.now));
  const BM &icon = now_bitmap(m.now_icon);
  bitmap(d, icon, cx - bm_w(icon) * 2, y + 16, 4, ink());
  auto t = wrap_n(f_vt_, m.now_title, 100, 2);
  for (size_t i = 0; i < t.size(); i++)
    text(d, f_vt_, cx, y + 74 + (int) i * 20, ink(), t[i], Align::CENTER);
  int yb = y + 74 + (int) t.size() * 20 + 4;
  text(d, f_vt_, cx, yb, ink(105), fit(f_vt_, m.now_body, 100), Align::CENTER);
  if (yb + 20 < y + h - 6)
    text(d, f_vt_, cx, yb + 20, ink(105), fit(f_vt_, m.now_detail, 100), Align::CENTER);
}

// ------------------------------------------------------------------ LISTEN
// A microphone window with chunky level bars (two samples per bar), the
// elapsed time ticking in the title bar like a recorder.
void PixelSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  this->titlebar_(d, upper(T.listening), "REC " + mmss(m.listen_seconds));
  this->win_(d, 8, 30, 384, 150, upper(T.mic));
  const size_t n = m.mic_history.size() / 2;
  for (size_t i = 0; i < n; i++) {
    float v = std::max(m.mic_history[2 * i], m.mic_history[2 * i + 1]);
    int units = (int) lroundf(clampf(v, 0, 1) * 14);
    int hgt = 4 + units * 8;
    fill_rect(d, 16 + (int) i * 8, 104 - units * 4, 6, hgt, ink());
  }
  text(d, f_vt_, 16, 206, ink(), fit(f_vt_, std::string(T.speak_naturally) + ".", 368));
  text(d, f_vt_, 16, 228, ink(105), fit(f_vt_, T.stop_when_quiet, 368));
  this->button_(d, 164, 272, 'B', 13, T.finish, 20, 7);
  this->button_(d, 288, 272, 'v', 13, T.cancel, 20, 7);
}

// ------------------------------------------------------------------ THINK
// Quest log: what was heard, then the three steps as checkboxes.
void PixelSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  this->titlebar_(d, upper(T.thinking_title), hhmm(m.hour, m.minute));
  this->win_(d, 14, 34, 372, 72, upper(T.heard));
  auto q = wrap_n(f_vt_, "> " + (m.question.empty() ? std::string("...") : m.question), 344, 2);
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_vt_, 26, 62 + (int) i * 24, ink(), q[i]);

  this->win_(d, 14, 118, 372, 126, upper(T.progress));
  int st = clampi(m.pipeline_step, 0, 2);
  for (int i = 0; i < 3; i++) {
    int y = 152 + i * 30;
    const BM &b = i < st ? BM_BOX_OK : (i == st ? BM_PLAY : BM_BOX);
    bitmap(d, b, 28, y - 15, 2, ink());
    std::string l = T.steps[i];
    if (i == st)
      l += "...";
    text(d, f_vt_, 52, y, ink(), l);
  }
  this->button_(d, 288, 272, 'v', 13, T.cancel, 20, 7);
}

// ------------------------------------------------------------------ ANSWER
void PixelSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  this->titlebar_(d, upper(T.assistant), hhmm(m.hour, m.minute));
  this->win_(d, 14, 28, 372, 42, "");
  text(d, f_vt_, 26, 56, ink(), fit(f_vt_, "> " + m.question, 348));

  bool facts = !m.facts.empty();
  int wh = facts ? 168 : 212;
  this->win_(d, 14, 80, 372, wh, upper(T.answer));
  AnswerLayout a = layout_answer(f_vt_, m.answer, 344, this->answer_lines_(m), m.answer_words_shown);
  for (size_t i = 0; i < a.lines.size(); i++)
    text(d, f_vt_, 26, 110 + (int) i * 24, ink(), a.lines[i]);
  if (a.cur >= 0) {
    const std::string &l = a.lines[a.cur];
    int x = l.empty() ? 26 : std::min(26 + measure(f_vt_, l) + 4, 370);
    fill_rect(d, x, 110 + a.cur * 24 - 16, 10, 18, ink());
  } else if (a.more) {
    bitmap(d, BM_ARROW, 360, 80 + wh - 18, 2, ink());
  }

  for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
    int x = 14 + (int) i * 126;
    this->win_(d, x, 256, 120, 40, "");
    std::string lab = lower(m.facts[i].second), v = m.facts[i].first;
    const BM *ic = i == 0 ? &BM_UMBRELLA : (i == 1 ? &BM_DROP : &BM_THERMO);
    if (contains_any(lab, T.kw_rain))
      ic = &BM_UMBRELLA;
    else if (contains_any(lab, T.kw_precip) || v.find("mm") != std::string::npos)
      ic = &BM_DROP;
    else if (contains_any(lab, T.kw_temp) || v.find("°") != std::string::npos)
      ic = &BM_THERMO;
    else if (!std::isnan(parse_hm(v)))  // a time of day
      ic = &BM_CLOCK;
    bitmap(d, *ic, x + 12, 266, 2, ink());
    text(d, f_vt_, x + 38, 284, ink(), fit(f_vt_, v, 76));
  }
}

// ------------------------------------------------------------------ WEATHER
void PixelSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  this->titlebar_(d, upper(T.weather) + ": " + upper(T.tomorrow), short_day(T, m.fc_day) + " " + ddmm(m.fc_date));
  float total = fc_rain_total(m);
  if (total >= 0.2f) {
    bitmap(d, BM_CLOUD, 16, 34, 4, ink());
    bitmap(d, BM_CLOUD_IN, 16, 34, 4, paper());
    bitmap(d, BM_RAIN, 18, 74, 4, ink());
  } else {
    bitmap(d, BM_SUN, 20, 30, 5, ink());
  }
  text(d, f_l_, 96, 70, ink(), temp_str(m.fc_hi, true));
  text(d, f_vt_, 96, 96, ink(), "min " + temp_str(m.fc_lo, true));
  text(d, f_vt_, 96, 118, ink(), fit(f_vt_, lower(m.fc_summary), 288));
  text(d, f_m_, 384, 70, ink(), mm_str(total, false, T.decimal_sep), Align::RIGHT);
  text(d, f_vt_, 384, 92, ink(105), lower(T.precip), Align::RIGHT);

  float lo, hi;
  fc_temp_range(m, lo, hi);
  bool direct = lo >= 0 && hi <= 9.4f;  // one block per degree
  for (int i = 0; i < 18; i++) {
    int h = 6 + i, x = 22 + i * 20;
    float t = m.fc_temp[i];
    if (!std::isnan(t)) {
      int n = direct ? (int) lroundf(t) : 1 + (int) lroundf((t - lo) / std::max(hi - lo, 1.0f) * 8);
      n = clampi(n, 0, 9);
      for (int b = 0; b < n; b++)
        fill_rect(d, x, 200 - (b + 1) * 8 + 1, 16, 7, ink());
    }
    int r = clampi((int) lroundf(fc_rain_at(m, h) / 0.2f), 0, 5);
    for (int b = 0; b < r; b++)
      checker(d, x, 205 + b * 8, 16, 7);
    if (h % 3 == 0)
      text(d, f_s_, x + 8, 256, ink(), pad2(h), Align::CENTER);
  }
  fill_rect(d, 20, 201, 362, 2, ink());

  std::vector<std::string> p;
  if (!m.fc_wind.empty())
    p.push_back(lower(T.wind) + " " + m.fc_wind);
  if (!m.fc_humidity.empty())
    p.push_back(std::string(T.humidity_abbr) + " " + m.fc_humidity);
  if (!m.fc_sunset.empty())
    p.push_back(std::string(T.sunset_abbr) + " " + m.fc_sunset);
  text(d, f_vt_, 16, 288, ink(), fit(f_vt_, join_dot(p, "  "), 368));
}

// ------------------------------------------------------------------ TIMER
void PixelSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  this->titlebar_(d, upper(T.timer) + ": " + upper(m.timer_name), hhmm(m.hour, m.minute));
  text(d, f_xxl_, 200, 132, ink(), mmss(m.timer_remaining), Align::CENTER);
  text(d, f_s_, 20, 176, ink(), fit(f_s_, upper(T.time_label), 40));
  stroke_rect(d, 64, 158, 312, 24, ink(), 2);
  int on = (int) lroundf(timer_frac(m) * 30);
  for (int i = 0; i < 30; i++) {
    int x = 69 + i * 10;
    if (i < on)
      fill_rect(d, x, 163, 8, 14, ink());
    else
      checker(d, x, 163, 8, 14);
  }
  if (!m.timer_end.empty())
    text(d, f_vt_, 200, 220, ink(), subst(T.ends_at, m.timer_end), Align::CENTER);
  this->button_(d, 40, 272, 'A', 13, "+1 min", 20, 7);
  this->button_(d, 164, 272, 'B', 13, T.stop, 20, 7);
  this->button_(d, 288, 272, 'v', 13, T.pause, 20, 7);
}

// ------------------------------------------------------------------ DOORBELL
void PixelSkin::doorbell_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  if (!bell_image(d, m, 264)) {
    checker(d, 0, 0, 264, 300);
    this->win_(d, 22, 120, 220, 60, "");
    text(d, f_vt_, 132, 157, ink(), fit(f_vt_, lower(T.waiting_image), 200), Align::CENTER);
  }
  std::string cap = upper(T.camera) + ": " + upper(m.bell_place);
  fill_rect(d, 6, 6, measure(f_s_, cap) + 8, 14, ink());
  text(d, f_s_, 10, 17, paper(), cap, Align::LEFT, ink());

  fill_rect(d, 264, 0, 136, 300, paper());
  fill_rect(d, 264, 0, 2, 300, ink());
  this->win_(d, 270, 8, 124, 104, upper(T.doorbell));
  bitmap(d, BM_BELL, 314, 26, 4, ink());
  text(d, f_m_, 332, 92, ink(), m.bell_time.empty() ? hhmm(m.hour, m.minute) : m.bell_time, Align::CENTER);

  this->win_(d, 270, 120, 124, 108, "");
  text(d, f_s_, 280, 138, ink(), fit(f_s_, upper(m.bell_place), 104));
  auto c = wrap_n(f_vt_, m.bell_caption.empty() ? lower(T.someone_rang) : m.bell_caption, 104, 3);
  for (size_t i = 0; i < c.size(); i++)
    text(d, f_vt_, 280, 162 + (int) i * 20, ink(), c[i]);

  this->button_(d, 284, 244, 'A', 9, T.open_door, 16, 7);
  this->button_(d, 284, 266, 'B', 9, T.talk, 16, 7);
  this->button_(d, 284, 288, 'v', 9, T.close_door, 16, 7);
}

}  // namespace note4_ui
}  // namespace esphome
