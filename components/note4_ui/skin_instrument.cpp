// Skin "instrument". Ported from note4-style.html (idleC, answerC, weatherC,
// timerC). LISTEN is an oscilloscope field, THINK a process table, DOORBELL a
// labelled side panel, all with the same tags, dot grid and corner marks.
#include "skin_instrument.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "ui_util.h"

namespace esphome {
namespace note4_ui {

using namespace uu;

namespace {

const Color K = ink(0);
const Color WH = ink(255);
// The mockup's dot grid is grey 150 single pixels. As a tone they would all
// land on the same Bayer cell and vanish on a 1-bit refresh, so they are ink:
// black on 1-bit, mid grey on 16 grey.
const Color DOT = ink(115);

const int ANS_Y = 108, ANS_PITCH = 26, ANS_W = 368;

std::string pad2(int v) {
  char b[16];
  snprintf(b, sizeof(b), "%02d", v);
  return b;
}

std::string deg_c(float v) { return std::isnan(v) ? "–" : signed_deg(v) + "C"; }
std::string num(float v) { return std::isnan(v) ? "–" : signed_int((int) std::lround(v)); }

void grid_dots(Display &d) {
  for (int y = 30; y < 296; y += 8)
    for (int x = 12; x < 390; x += 8)
      d.draw_pixel_at(x, y, DOT);
}

void corners(Display &d, bool left = true) {
  struct C {
    int x, y, dx, dy;
  };
  static const C cs[4] = {{3, 3, 1, 1}, {396, 3, -1, 1}, {3, 296, 1, -1}, {396, 296, -1, -1}};
  for (const auto &c : cs) {
    if (!left && c.dx > 0)
      continue;
    fill_rect(d, c.dx > 0 ? c.x : c.x - 7, c.y, 8, 1, K);
    fill_rect(d, c.x, c.dy > 0 ? c.y : c.y - 7, 1, 8, K);
  }
}

void clear(Display &d, int x, int y, int w, int h) { fill_rect(d, x, y, w, h, paper()); }

}  // namespace

// ---------------------------------------------------------------------------
int InstrumentSkin::tag_(Display &d, const std::string &txt, int x, int y, bool inv, bool on_black) {
  const int w = measure(f_.m9s, txt) + 8;
  const Color ink_c = on_black ? WH : K, paper_c = on_black ? K : paper();
  if (inv) {
    fill_rect(d, x, y - 10, w, 13, ink_c);
    text(d, f_.m9s, x + 4, y, paper_c, txt, Align::LEFT, ink_c);
  } else {
    stroke_rect(d, x, y - 10, w, 13, ink_c);
    text(d, f_.m9s, x + 4, y, ink_c, txt, Align::LEFT, paper_c);
  }
  return w;
}

void InstrumentSkin::module_(Display &d, int x, int w, const std::string &title) {
  clear(d, x, 188, w, 100);
  stroke_rect(d, x, 188, w, 100, K);
  fill_rect(d, x, 188, w, 14, K);
  text(d, f_.m9s, x + 5, 198, WH, title, Align::LEFT, K);
}

void InstrumentSkin::status_(Display &d, const Model &m, const std::string &left) {
  const Strings &T = *m.tr;
  text(d, f_.m10s, 16, 16, K, fit(f_.m10s, left, 280), Align::LEFT);
  std::string pct = m.battery >= 0 ? std::to_string(m.battery) + "%" : "–";
  text(d, f_.m10s, 384, 16, K, (m.charging ? upper(T.charging_abbr) : std::string("BAT")) + " " + pct, Align::RIGHT);
  static const int HS[4] = {3, 5, 7, 9};
  for (int i = 0; i < 4; i++) {
    if (m.wifi || i == 0)
      fill_rect(d, 316 + i * 4, 16 - HS[i], 3, HS[i], K);
    else
      stroke_rect(d, 316 + i * 4, 16 - HS[i], 3, HS[i], K);
  }
  if (!m.ha)
    text(d, f_.m10s, 310, 16, K, "HA OFF", Align::RIGHT);
  dotted_hline(d, 12, 390, 23, K);
}

void InstrumentSkin::frame_(Display &d, const Model &m, const std::string &left) {
  grid_dots(d);
  corners(d);
  status_(d, m, left);
}

void InstrumentSkin::draw(Display &d, Screen s, const Model &m) {
  switch (s) {
    case Screen::IDLE:
      idle_(d, m);
      break;
    case Screen::LISTEN:
      listen_(d, m);
      break;
    case Screen::THINK:
      think_(d, m);
      break;
    case Screen::ANSWER:
      answer_(d, m);
      break;
    case Screen::WEATHER:
      weather_(d, m);
      break;
    case Screen::TIMER:
      timer_(d, m);
      break;
    case Screen::DOORBELL:
      doorbell_(d, m);
      break;
  }
}

int InstrumentSkin::answer_lines_(const Model &m) const { return m.facts.empty() ? 7 : 5; }

std::vector<Rect> InstrumentSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{10, 44, 238, 66}};
      if (m.timer_active)
        r.push_back({258, 186, 128, 104});
      return r;
    }
    case Screen::LISTEN:
      return {{12, 58, 376, 130}, {290, 30, 98, 26}};
    case Screen::THINK:
      return {{12, 110, 376, 92}};
    case Screen::ANSWER: {
      AnswerLayout L = layout_answer(f_.answer, m, ANS_W, answer_lines_(m));
      return typing_rects(L, 12, 376, ANS_Y, ANS_PITCH, 20, 27);
    }
    case Screen::TIMER:
      return {{34, 60, 210, 212}};
    default:
      return {};
  }
}

// ---------------------------------------------------------------------------
void InstrumentSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, "NOTE4 · " + upper(m.room) + " · " + ddmm(m.date));
  clear(d, 12, 30, 240, 84);
  tag_(d, "[01] " + upper(T.time_label), 16, 42);
  text(d, f_.clock, 13, 104, K, hhmm(m.hour, m.minute), Align::LEFT, paper(), -3);

  clear(d, 258, 30, 132, 92);
  tag_(d, fit(f_.m9s, "[02] " + upper(T.conditions), 120), 262, 42);
  const std::string rows[5][2] = {
      {upper(T.day), upper(weekday_short(T, m.weekday))},
      {"TEMP", deg_c(m.temp_now)},
      {"MIN/MAX", num(m.temp_lo) + "/" + num(m.temp_hi)},
      {upper(T.sky), upper(first_clause(m.condition))},
      {upper(T.tomorrow), num(m.fc_lo) + "/" + num(m.fc_hi)},
  };
  for (int i = 0; i < 5; i++) {
    const int y = 60 + i * 15;
    text(d, f_.m10, 262, y, grey(115), rows[i][0]);
    int lw = measure(f_.m10, rows[i][0]);
    text(d, f_.m10s, 386, y, K, fit(f_.m10s, rows[i][1], 124 - lw - 6), Align::RIGHT);
  }
  dotted_vline(d, 252, 30, 120, K);

  // [03] day 06-22
  auto X = [](float h) { return (int) std::lround(16 + (h - 6) / 16 * 368); };
  clear(d, 12, 120, 378, 62);
  tag_(d, "[03] " + upper(T.day) + " 06–22", 16, 132);
  for (size_t i = 0; i < m.events.size(); i++) {
    const auto &e = m.events[i];
    float a = std::max(6.0f, e.start_h), b = std::min(22.0f, e.end_h);
    if (b <= a)
      continue;
    int x0 = X(a), w = std::max(2, X(b) - x0);
    if (b - a > 1.0f) {
      fill_rect(d, x0, 148, w, 9, tone(140));
      stroke_rect(d, x0, 148, w, 9, K);
    } else {
      fill_rect(d, x0, 148, w, 9, K);
    }
    int room = 384 - x0;
    for (size_t j = 0; j < m.events.size(); j++)
      if (j != i && m.events[j].start_h > e.start_h && m.events[j].start_h <= 22)
        room = std::min(room, X(std::max(6.0f, m.events[j].start_h)) - x0 - 4);
    if (room > 12)
      text(d, f_.m8s, x0, 144, K, fit(f_.m8s, upper(e.title), room));
  }
  hline(d, 16, 384, 160, K);
  for (int h = 6; h <= 22; h++) {
    vline(d, X(h), 160, h % 3 ? 163 : 167, K);
    if (h % 3 == 0)
      text(d, f_.m9, X(h), 177, K, pad2(h), Align::CENTER);
  }
  const int nx = X(now_hours(m));
  vline(d, nx, 136, 168, K);
  fill_rect(d, nx - 2, 134, 5, 3, K);

  // [04] next
  module_(d, 16, 116, fit(f_.m9s, "[04] " + upper(T.next), 106));
  if (!m.next_time.empty() || !m.next_title.empty()) {
    text(d, f_.next, 22, 232, K, m.next_time, Align::LEFT, paper(), -1);
    auto t = wrap_lines(f_.m10s, upper(m.next_title), 104, 2);
    for (size_t i = 0; i < t.size(); i++)
      text(d, f_.m10s, 22, 250 + (int) i * 13, K, t[i]);
    std::string cd = countdown(m);
    text(d, f_.m10, 22, 280, grey(110), fit(f_.m10, cd.empty() ? upper(m.next_in) : cd, 104));
  } else {
    text(d, f_.next, 22, 232, K, "–", Align::LEFT, paper(), -1);
    text(d, f_.m10s, 22, 250, K, fit(f_.m10s, upper(T.no_events_1), 104));
    text(d, f_.m10s, 22, 263, K, fit(f_.m10s, upper(T.no_events_2), 104));
  }

  now_module_(d, m);

  // [06] timer, or tomorrow
  if (m.timer_active) {
    module_(d, 260, 124, "[06] " + upper(T.timer));
    text(d, f_.card, 266, 234, K, mmss(m.timer_remaining), Align::LEFT, paper(), -1);
    text(d, f_.m10s, 266, 252, K, fit(f_.m10s, upper(m.timer_name), 112));
    const int on = (int) std::ceil(timer_frac(m) * 10 - 1e-4f);
    for (int i = 0; i < 10; i++) {
      const int x = 266 + i * 11;
      if (i < on)
        fill_rect(d, x, 264, 9, 14, K);
      else
        stroke_rect(d, x, 264, 9, 14, K);
    }
  } else {
    module_(d, 260, 124, fit(f_.m9s, "[06] " + upper(T.tomorrow) + " · " + upper(weekday_short(T, m.fc_day)), 114));
    text(d, f_.card, 266, 234, K, num(m.fc_lo) + "/" + num(m.fc_hi), Align::LEFT, paper(), -1);
    text(d, f_.m8s, 266, 248, grey(110), "MIN/MAX °C");
    auto t = wrap_lines(f_.m10s, upper(m.fc_summary), 112, 2);
    for (size_t i = 0; i < t.size(); i++)
      text(d, f_.m10s, 266, 264 + (int) i * 13, K, t[i]);
  }
}

// [05] The "Now" card: TODO as the mockup (tag + two lines + source), ALARM
// the same in negative, BRIEF the sentence in plain mono.
void InstrumentSkin::now_module_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const int x = 138, w = 116, tx = 144, tw = 104;
  if (m.now_level == NowLevel::BRIEF) {
    module_(d, x, w, "[05] " + upper(T.day));
    auto lines = wrap_lines(f_.m10s, brief_text(m), tw, 5);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_.m10s, tx, 218 + (int) i * 14, K, lines[i]);
    return;
  }
  const bool alarm = m.now_level == NowLevel::ALARM;
  module_(d, x, w, fit(f_.m9s, "[05] " + upper(alarm ? T.alarm : T.now) + (alarm ? "  !2" : "  !1"), 106));
  const Color fg = alarm ? WH : K, bg = alarm ? K : paper();
  if (alarm)
    fill_rect(d, x, 202, w, 86, K);
  tag_(d, fit(f_.m9s, upper(now_kind(T, m.now_icon)), tw - 8), tx, 222, true, alarm);
  text(d, f_.m12s, tx, 242, fg, fit(f_.m12s, upper(m.now_title), tw), Align::LEFT, bg);
  text(d, f_.m12s, tx, 258, fg, fit(f_.m12s, upper(m.now_body), tw), Align::LEFT, bg);
  text(d, f_.m8s, tx, 280, alarm ? WH : grey(110), fit(f_.m8s, upper(m.now_detail), tw), Align::LEFT, bg);
}

// ---------------------------------------------------------------------------
void InstrumentSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, hhmm(m.hour, m.minute) + " · " + upper(m.room));
  clear(d, 12, 30, 378, 260);
  const int w = tag_(d, "IN", 16, 46);
  text(d, f_.m12s, 16 + w + 8, 46, K, fit(f_.m12s, upper(m.question), 368 - w - 8));
  dotted_hline(d, 16, 384, 58, K);
  tag_(d, "OUT", 16, 80);
  AnswerLayout L = layout_answer(f_.answer, m, ANS_W, answer_lines_(m));
  for (size_t i = 0; i < L.lines.size(); i++)
    text(d, f_.answer, 16, ANS_Y + (int) i * ANS_PITCH, K, L.lines[i]);
  if (L.typing)
    fill_rect(d, 16 + L.cursor_w + (L.cursor_w > 0 ? 4 : 0), ANS_Y + L.cursor_line * ANS_PITCH - 15, 9, 19, K);
  if (!m.facts.empty()) {
    stroke_rect(d, 16, 224, 368, 35, K);
    for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
      const int x = 16 + (int) i * 123;
      if (i)
        vline(d, x, 225, 258, K);
      text(d, f_.m8s, x + 7, 237, grey(110), fit(f_.m8s, upper(m.facts[i].second), 110));
      text(d, f_.m13s, x + 7, 252, K, fit(f_.m13s, upper(m.facts[i].first), 110));
    }
  }
  text(d, f_.m9, 16, 287, grey(110), "SRC HOME ASSISTANT");
  keycap(d, f_.m10s, Sym::DOWN, upper(T.details), 384, 288, Align::RIGHT);
}

// ---------------------------------------------------------------------------
void InstrumentSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, hhmm(m.hour, m.minute) + " · " + upper(T.weather));
  clear(d, 12, 30, 378, 82);
  tag_(d, "[" + upper(T.forecast) + "] " + upper(T.tomorrow) + " · " + upper(weekday_short(T, m.fc_day)) + " " + ddmm(m.fc_date),
       16, 42);
  text(d, f_.fc_hi, 13, 98, K, std::isnan(m.fc_hi) ? "–" : signed_deg(m.fc_hi), Align::LEFT, paper(), -3);
  const std::string rows[4][2] = {
      {"MIN", deg_c(m.fc_lo)},
      {upper(T.precip), fmt1(fc_rain_sum(m)) + " MM"},
      {upper(T.wind), upper(m.fc_wind)},
      {upper(T.humidity_abbr) + ".", upper(m.fc_humidity)},
  };
  for (int i = 0; i < 4; i++) {
    const int y = 60 + i * 15;
    text(d, f_.m10, 176, y, grey(115), rows[i][0]);
    text(d, f_.m10s, 384, y, K, fit(f_.m10s, rows[i][1], 160), Align::RIGHT);
  }

  // chart
  const int px0 = 40, px1 = 384, py0 = 118, py1 = 238;
  auto X = [&](float h) { return px0 + (h - 6) / 18 * (px1 - px0); };
  float lo, hi;
  fc_range(m, lo, hi);
  const float g0 = std::floor(lo / 4) * 4;
  float step = 4;
  while (g0 + 2 * step < hi)
    step += 2;
  auto Y = [&](float v) { return 230 - (v - g0) / (2 * step) * 104; };
  clear(d, 12, py0 - 4, 378, 150);
  for (int k = 0; k < 3; k++) {
    float v = g0 + k * step;
    int y = (int) std::lround(Y(v));
    dotted_hline(d, px0, px1, y, K);
    text(d, f_.m9, 16, y + 3, K, signed_int((int) v));
  }
  for (int h = 9; h <= 21; h += 3)
    dotted_vline(d, (int) std::lround(X(h)), py0, py1, K);
  // rain: hatched bars
  const float mx = std::max(0.7f, fc_rain_max(m));
  for (int i = 0; i < 18; i++) {
    float mm = m.fc_rain[i];
    if (std::isnan(mm) || mm < 0.05f)
      continue;
    const int x = (int) std::lround(X(6.0f + i)) + 2, bh = std::max(1, (int) std::lround(mm / mx * 40));
    for (int y = py1 - bh; y < py1; y++)
      for (int xx = x; xx < x + 15; xx++)
        if (((xx - y) & 3) == 0)
          d.draw_pixel_at(xx, y, K);
    stroke_rect(d, x, py1 - bh, 15, bh, K);
  }
  // temperature as a step line, 2 px
  int prev_y = 0;
  for (int h = 6; h < 24; h++) {
    const int y = (int) std::lround(Y(std::round(fc_temp_at(m, h + 0.5f))));
    const int xa = (int) std::lround(X(h)), xb = (int) std::lround(X(h + 1));
    if (h > 6)
      fill_rect(d, xa - 1, std::min(prev_y, y) - 1, 2, std::abs(y - prev_y) + 2, K);
    fill_rect(d, xa - 1, y - 1, xb - xa + 2, 2, K);
    prev_y = y;
  }
  stroke_rect(d, px0, py0, px1 - px0, py1 - py0, K);
  if (!std::isnan(m.fc_rain_start_h)) {
    const int rx = (int) std::lround(X(m.fc_rain_start_h));
    vline(d, rx, py0, py1, K);
    float sh = m.fc_rain_start_h;
    std::string t = upper(T.rain) + " " + hhmm(((int) sh) % 24, (int) std::lround((sh - (int) sh) * 60) % 60);
    int tw = measure(f_.m9s, t) + 8;
    tag_(d, t, rx + 4 + tw <= px1 ? rx + 4 : rx - 4 - tw, py0 + 14);
  }
  for (int h = 6; h <= 24; h += 3)
    text(d, f_.m9, (int) std::lround(X(h)), 252, K, pad2(h), Align::CENTER);
  // Value after its label; longer labels (other languages) push it right.
  const std::string rise = fit(f_.m9, upper(T.sunrise), 60), set = fit(f_.m9, upper(T.sunset), 60);
  text(d, f_.m9, 16, 284, grey(115), rise);
  text(d, f_.m10s, std::max(62, 16 + measure(f_.m9, rise) + 6), 284, K, m.fc_sunrise);
  text(d, f_.m9, 120, 284, grey(115), set);
  text(d, f_.m10s, std::max(166, 120 + measure(f_.m9, set) + 6), 284, K, m.fc_sunset);
  text(d, f_.m9, 228, 284, grey(115), fit(f_.m9, upper(T.summary_label), 32));
  text(d, f_.m10s, 262, 284, K, fit(f_.m10s, upper(first_clause(m.fc_summary)), 122));
}

// ---------------------------------------------------------------------------
void InstrumentSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, hhmm(m.hour, m.minute) + " · " + upper(T.timer));
  clear(d, 12, 30, 378, 262);
  tag_(d, fit(f_.m9s, "[" + upper(T.timer) + "] " + upper(m.timer_name), 230), 16, 44);
  const float cx = 138, cy = 166, R = 98;
  const float fr = timer_frac(m);
  const int on = (int) std::ceil(fr * 60 - 1e-4f);
  for (int i = 0; i < 60; i++) {
    const float a = TOP + i * 3.14159265f / 30;
    const bool major = i % 6 == 0, lit = i < on;
    const float len = major ? 16 : 9;
    const float w = lit ? (major ? 3 : 2) : 1;
    const float inner = lit ? R - len : R - (major ? 8 : 4);
    thick_line(d, cx + std::cos(a) * R, cy + std::sin(a) * R, cx + std::cos(a) * inner, cy + std::sin(a) * inner, w, K);
  }
  const std::string total = m.timer_total % 60 == 0 ? std::to_string(m.timer_total / 60) : mmss(m.timer_total);
  text(d, f_.m9s, (int) cx, (int) (cy - R - 6), K, total, Align::CENTER);
  text(d, f_.timer, (int) cx, (int) cy + 14, K, mmss(m.timer_remaining), Align::CENTER, paper(), -2);
  text(d, f_.m9s, (int) cx, (int) cy + 34, K, "MM:SS", Align::CENTER);
  dotted_vline(d, 262, 64, 268, K);
  const std::string rows[4][2] = {
      {"START", m.timer_start},
      {upper(T.end), m.timer_end},
      {upper(T.total), mmss(m.timer_total)},
      {upper(T.left), std::to_string((int) std::lround(fr * 100)) + " %"},
  };
  for (int i = 0; i < 4; i++) {
    const int y = 82 + i * 44;
    text(d, f_.m9, 276, y, grey(115), rows[i][0]);
    text(d, f_.m20s, 276, y + 22, K, rows[i][1], Align::LEFT, paper(), -1);
  }
  keycap(d, f_.m10s, Sym::UP, "+1MIN", 276, 289, Align::LEFT);
  keycap(d, f_.m10s, Sym::DOT, upper(T.stop), 384, 289, Align::RIGHT);
}

// ---------------------------------------------------------------------------
// LISTEN: an oscilloscope field with the voice level, elapsed time T+.
void InstrumentSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, hhmm(m.hour, m.minute) + " · " + upper(m.room));
  clear(d, 12, 30, 378, 262);
  tag_(d, "[REC] " + upper(T.listening), 16, 44);
  fill_circle(d, 296, 42, 3.5f, K);
  text(d, f_.m20s, 384, 50, K, "T+" + mmss(m.listen_seconds), Align::RIGHT, paper(), -1);
  stroke_rect(d, 16, 62, 368, 122, K);
  dotted_hline(d, 17, 383, 123, K);
  for (int k = 1; k < 8; k++)
    dotted_vline(d, 16 + k * 46, 63, 183, K);
  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float l = std::min(1.0f, std::max(0.0f, m.mic_history[i]));
    int h = std::max(2, (int) std::lround(2 + l * 108));
    fill_rect(d, 17 + (int) i * 4, (int) std::lround(123 - h / 2.0f), 2, h, K);
  }
  text(d, f_.m8s, 16, 196, grey(110), upper(T.mic_level));
  const std::string rows[3][2] = {
      {"WAKE", upper(m.wake_word)}, {upper(T.room), upper(m.room)}, {upper(T.end), upper(T.after_silence)}};
  for (int i = 0; i < 3; i++) {
    const int y = 216 + i * 15;
    text(d, f_.m10, 16, y, grey(115), rows[i][0]);
    text(d, f_.m10s, 100, y, K, fit(f_.m10s, rows[i][1], 280));
  }
  text(d, f_.m10, 16, 266, grey(115), fit(f_.m10, upper(std::string(T.speak_naturally) + ". " + T.stop_when_quiet), 368));
  keycap(d, f_.m10s, Sym::DOT, upper(T.finish), 16, 289, Align::LEFT);
  keycap(d, f_.m10s, Sym::DOWN, upper(T.cancel), 384, 289, Align::RIGHT);
}

// THINK: the heard question tagged IN, the pipeline as a process table.
void InstrumentSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  frame_(d, m, hhmm(m.hour, m.minute) + " · " + upper(m.room));
  clear(d, 12, 30, 378, 262);
  tag_(d, "[PROC] " + upper(T.thinking), 16, 44);
  const int w = tag_(d, "IN", 16, 72);
  auto q = wrap_lines(f_.m12s, upper(m.question), 368 - w - 8, 2);
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_.m12s, 16 + w + 8, 72 + (int) i * 16, K, q[i]);
  dotted_hline(d, 16, 384, 100, K);
  for (int i = 0; i < 3; i++) {
    const int y = 132 + i * 30;
    const bool done = i < m.pipeline_step, active = i == m.pipeline_step;
    const Color fg = active ? WH : K, bg = active ? K : paper();
    if (active)
      fill_rect(d, 16, y - 17, 368, 24, K);
    else
      dotted_hline(d, 16, 384, y + 7, K);
    text(d, f_.m10s, 22, y, fg, "[" + std::to_string(i + 1) + "]", Align::LEFT, bg);
    text(d, f_.m12s, 56, y, fg, upper(T.steps[i]), Align::LEFT, bg);
    text(d, f_.m10s, 378, y, active ? WH : (done ? K : grey(115)), done ? upper(T.step_done) : active ? upper(T.step_running) : std::string("–"),
         Align::RIGHT, bg);
  }
  text(d, f_.m9, 16, 287, grey(110), "SRC HOME ASSISTANT · ASSIST");
  keycap(d, f_.m10s, Sym::DOWN, upper(T.cancel), 384, 288, Align::RIGHT);
}

// DOORBELL: snapshot left, a labelled panel right.
void InstrumentSkin::doorbell_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  if (m.bell_image != nullptr && m.bell_image_ready) {
    d.start_clipping(0, 0, 264, H);
    d.image(0, 0, m.bell_image);
    d.end_clipping();
  } else {
    fill_rect(d, 0, 0, 264, H, tone(190));
    text(d, f_.m10s, 132, 154, K, upper(T.waiting_image) + "…", Align::CENTER, tone(190));
  }
  tag_(d, fit(f_.m9s, "[CAM] " + upper(m.bell_place), 240), 8, 20);
  clear(d, 264, 0, 136, H);
  vline(d, 264, 0, H, K);
  corners(d, false);
  const int x = 276;
  tag_(d, fit(f_.m9s, "[" + upper(T.doorbell) + "]", 104), x, 22);
  text(d, f_.m9, x, 46, grey(115), upper(T.clock_time));
  text(d, f_.card, x - 1, 76, K, m.bell_time, Align::LEFT, paper(), -1);
  dotted_hline(d, x, 388, 88, K);
  auto cap = wrap_lines(f_.m12s, upper(m.bell_caption), 112, 4);
  for (size_t i = 0; i < cap.size(); i++)
    text(d, f_.m12s, x, 108 + (int) i * 16, K, cap[i]);
  if (!cap.empty())
    text(d, f_.m8s, x, 176, grey(110), fit(f_.m8s, upper(T.caption_source), 112));
  dotted_hline(d, x, 388, 186, K);
  keycap(d, f_.m10s, Sym::UP, upper(T.open_door), x, 214, Align::LEFT);
  keycap(d, f_.m10s, Sym::DOT, upper(T.talk), x, 240, Align::LEFT);
  keycap(d, f_.m10s, Sym::DOWN, upper(T.close_door), x, 266, Align::LEFT);
}

}  // namespace note4_ui
}  // namespace esphome
