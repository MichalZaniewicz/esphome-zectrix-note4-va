// Skin "tiles". Ported from note4-style.html (idleD, answerD, weatherD,
// timerD). LISTEN is one big black tile with the voice in white, THINK three
// step tiles, DOORBELL a black info tile and pill buttons beside the snapshot.
//
// Live regions sit on white or black only, so the answer tile is white inside
// a grey ring (like the clock tile) instead of the mockup's flat grey 240:
// the line being typed is refreshed partially, in 1 bit.
#include "skin_tiles.h"

#include <cmath>

#include "ui_util.h"

namespace esphome {
namespace note4_ui {

using namespace uu;

namespace {

const Color K = ink(0);
const Color WH = ink(255);
const float PI_F = 3.14159265f;

const int ANS_X = 26, ANS_Y = 101, ANS_PITCH = 26, ANS_W = 344, ANS_LINES = 5;

void umbrella(Display &d, float cx, float cy, Color c) {
  wedge(d, cx, cy, 7, PI_F, 2 * PI_F, c);
  thick_line(d, cx, cy, cx, cy + 7, 1.6f, c);
  arc(d, cx - 2.2f, cy + 7, 2.2f, 1.6f, 0, PI_F, c);
}

// "Now" card pictograms, drawn in a 30x38 box at (x, y).
void now_icon(Display &d, const std::string &icon, float x, float y, Color c, Color bg) {
  if (icon == "plant") {
    fill_ellipse(d, x + 9, y + 10, 5.5f, 10, -0.6f, c);
    fill_ellipse(d, x + 23, y + 10, 5.5f, 10, 0.6f, c);
    fill_rect(d, (int) x + 15, (int) y + 12, 2, 9, c);
    fill_poly(d, {{x + 4, y + 21}, {x + 28, y + 21}, {x + 24, y + 38}, {x + 8, y + 38}}, c);
  } else if (icon == "window") {
    stroke_round(d, (int) x + 2, (int) y + 2, 26, 34, 3, c, 3);
    fill_rect(d, (int) x + 14, (int) y + 3, 2, 32, c);
    fill_rect(d, (int) x + 3, (int) y + 18, 24, 2, c);
    fill_poly(d, {{x + 17, y + 5}, {x + 26, y + 9}, {x + 26, y + 16}, {x + 17, y + 16}}, c);
  } else if (icon == "wind") {
    thick_line(d, x, y + 10, x + 22, y + 10, 3, c, true);
    arc(d, x + 22, y + 5, 5, 3, -PI_F / 2, PI_F / 2, c, true);
    thick_line(d, x + 4, y + 20, x + 28, y + 20, 3, c, true);
    thick_line(d, x, y + 30, x + 16, y + 30, 3, c, true);
    arc(d, x + 16, y + 35, 5, 3, -PI_F / 2, PI_F / 2, c, true);
  } else if (icon == "vacuum") {
    fill_circle(d, x + 15, y + 20, 14, c);
    ring(d, x + 15, y + 20, 8, 2, bg);
    fill_circle(d, x + 15, y + 20, 2.5f, bg);
  } else if (icon == "school") {
    fill_poly(d, {{x, y + 14}, {x + 15, y + 6}, {x + 30, y + 14}, {x + 15, y + 22}}, c);
    fill_rect(d, (int) x + 7, (int) y + 17, 16, 11, c);
    thick_line(d, x + 27, y + 15, x + 27, y + 27, 2, c);
    fill_circle(d, x + 27, y + 28, 2.5f, c);
  } else if (icon == "flood") {
    fill_poly(d, {{x + 6, y + 20}, {x + 24, y + 20}, {x + 15, y + 3}}, c);
    fill_circle(d, x + 15, y + 23, 9.5f, c);
    thick_line(d, x + 1, y + 36, x + 29, y + 36, 2.5f, c, true);
  } else if (icon == "smoke") {
    fill_circle(d, x + 10, y + 29, 7, c);
    fill_circle(d, x + 20, y + 22, 8, c);
    fill_circle(d, x + 13, y + 12, 6, c);
    fill_circle(d, x + 21, y + 6, 4, c);
  } else if (icon == "brief") {
    fill_circle(d, x + 15, y + 19, 8, c);
    for (int i = 0; i < 8; i++) {
      float a = i * PI_F / 4;
      thick_line(d, x + 15 + std::cos(a) * 11, y + 19 + std::sin(a) * 11, x + 15 + std::cos(a) * 15,
                 y + 19 + std::sin(a) * 15, 2, c, true);
    }
  } else {
    fill_round(d, (int) x + 12, (int) y + 3, 6, 23, 3, c);
    fill_circle(d, x + 15, y + 33, 3.5f, c);
  }
}

}  // namespace

// ---------------------------------------------------------------------------
void TilesSkin::status_(Display &d, const Model &m, const std::string &left) {
  text(d, f_.m12, 16, 17, K, left);
  fill_round(d, 350, 6, 36, 15, 7, K);
  std::string b = m.battery >= 0 ? std::to_string(m.battery) : "–";
  if (m.charging)
    b = "+" + b;
  text(d, f_.m10, 368, 17, WH, b, Align::CENTER, K);
  std::string off;
  if (!m.wifi)
    off = "offline";
  else if (!m.ha)
    off = lower(m.tr->without) + " HA";
  if (!off.empty())
    text(d, f_.m11m, 342, 17, grey(105), off, Align::RIGHT);
}

void TilesSkin::pill_(Display &d, int x, int y, int w, int h, const std::string &label, bool dark, Font *f) {
  const Color bg = dark ? K : tone(222);
  fill_round(d, x, y, w, h, h / 2, bg);
  text(d, f, x + w / 2, y + h / 2 + 5, dark ? WH : K, label, Align::CENTER, bg);
}

void TilesSkin::draw(Display &d, Screen s, const Model &m) {
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

std::vector<Rect> TilesSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{18, 40, 214, 74}};
      if (m.timer_active)
        r.push_back({264, 162, 128, 130});
      return r;
    }
    case Screen::LISTEN:
      return {{296, 42, 82, 38}, {14, 104, 372, 94}};
    case Screen::THINK:
      return {{8, 120, 384, 114}};
    case Screen::ANSWER: {
      AnswerLayout L = layout_answer(f_.answer, m, ANS_W, ANS_LINES);
      return typing_rects(L, 14, 372, ANS_Y, ANS_PITCH, 20, 27);
    }
    case Screen::TIMER:
      return {{90, 36, 220, 220}};
    default:
      return {};
  }
}

// ---------------------------------------------------------------------------
void TilesSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  // clock tile: white inside a grey ring
  fill_round(d, 10, 10, 228, 146, 18, tone(214));
  fill_round(d, 12, 12, 224, 142, 16, paper());
  text(d, f_.m12, 24, 34, grey(95), fit(f_.m12, m.room, 200));
  text(d, f_.clock, 21, 104, K, hhmm(m.hour, m.minute), Align::LEFT, paper(), -3);
  text(d, f_.m14m, 24, 136, grey(85), fit(f_.m14m, cap_first(m.weekday) + ", " + m.date, 206));

  // weather tile
  const Color wt = tone(56);
  fill_round(d, 246, 10, 144, 146, 18, wt);
  if (m.condition_icon == "partlycloudy" || m.condition_icon.empty()) {
    sun(d, 346, 44, 15, 252, 215, false);
    cloud(d, 310, 66, 40, 250, 196, false);
  } else {
    IconStyle st;
    st.outline = false;
    st.cloud_top = 250;
    st.cloud_bottom = 196;
    st.sun_inner = 252;
    st.sun_outer = 215;
    st.sun_rays = false;
    st.line = WH;
    st.line_w = 2.5f;
    st.round = true;
    weather_icon(d, m.condition_icon, 316, 58, 40, st);
  }
  text(d, f_.temp, 258, 128, WH, deg(m.temp_now), Align::LEFT, wt, -1);
  text(d, f_.m13, 378, 126, WH, deg(m.temp_hi) + " / " + deg(m.temp_lo), Align::RIGHT, wt);
  text(d, f_.m11m, 258, 146, WH, fit(f_.m11m, m.condition, 124), Align::LEFT, wt);

  // next
  const Color tt = tone(226), lab = tone(95);
  fill_round(d, 10, 164, 120, 126, 18, tt);
  text(d, f_.m11, 22, 186, lab, T.next, Align::LEFT, tt);
  if (!m.next_time.empty() || !m.next_title.empty()) {
    text(d, f_.next, 21, 220, K, m.next_time, Align::LEFT, tt, -1);
    auto t = wrap_lines(f_.m12m, m.next_title, 98, 2);
    for (size_t i = 0; i < t.size(); i++)
      text(d, f_.m12m, 22, 242 + (int) i * 15, K, t[i], Align::LEFT, tt);
    text(d, f_.m11m, 22, 276, lab, fit(f_.m11m, m.next_in, 98), Align::LEFT, tt);
  } else {
    text(d, f_.next, 21, 220, K, "–", Align::LEFT, tt, -1);
    text(d, f_.m12m, 22, 242, K, fit(f_.m12m, T.free_day, 98), Align::LEFT, tt);
    text(d, f_.m11m, 22, 276, lab, fit(f_.m11m, T.nothing_planned, 98), Align::LEFT, tt);
  }

  now_tile_(d, m);

  // timer, or tomorrow
  if (m.timer_active) {
    fill_round(d, 266, 164, 124, 126, 18, K);
    text(d, f_.m11, 278, 186, WH, T.timer, Align::LEFT, K);
    text(d, f_.card, 277, 226, WH, mmss(m.timer_remaining), Align::LEFT, K, -1);
    text(d, f_.m13, 278, 246, WH, fit(f_.m13, m.timer_name, 100), Align::LEFT, K);
    stroke_round(d, 278, 262, 101, 9, 4, WH, 1);
    int w = (int) std::lround(99 * timer_frac(m));
    if (w > 0)
      fill_round(d, 279, 263, w, 7, 3, WH);
  } else {
    fill_round(d, 266, 164, 124, 126, 18, tt);
    text(d, f_.m11, 278, 186, lab, T.tomorrow, Align::LEFT, tt);
    text(d, f_.next, 277, 220, K, deg(m.fc_hi) + " / " + deg(m.fc_lo), Align::LEFT, tt, -1);
    auto t = wrap_lines(f_.m12m, m.fc_summary, 100, 2);
    for (size_t i = 0; i < t.size(); i++)
      text(d, f_.m12m, 278, 242 + (int) i * 15, K, t[i], Align::LEFT, tt);
    text(d, f_.m11m, 278, 276, lab, fit(f_.m11m, m.fc_detail, 100), Align::LEFT, tt);
  }
}

// The "Now" card: TODO as the mockup (badge, pictogram, three lines), ALARM
// as a black tile, BRIEF the day's sentence.
void TilesSkin::now_tile_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const int x = 138, y = 164, w = 120, h = 126;
  const Color tt = tone(226);
  if (m.now_level == NowLevel::BRIEF) {
    fill_round(d, x, y, w, h, 18, tt);
    text(d, f_.m11, x + 12, y + 22, tone(95), T.today, Align::LEFT, tt);
    auto lines = wrap_lines(f_.m12m, brief_text(m), 98, 5);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_.m12m, x + 12, y + 44 + (int) i * 16, K, lines[i], Align::LEFT, tt);
    return;
  }
  const bool alarm = m.now_level == NowLevel::ALARM;
  const Color bg = alarm ? K : tt, fg = alarm ? WH : K;
  fill_round(d, x, y, w, h, 18, bg);
  text(d, f_.m11, x + 12, y + 22, alarm ? WH : tone(95), alarm ? T.alarm : T.now, Align::LEFT, bg);
  fill_round(d, x + 94, y + 10, 18, 18, 9, alarm ? WH : K);
  text(d, f_.m12, x + 103, y + 24, alarm ? K : WH, "!", Align::CENTER, alarm ? WH : K);
  now_icon(d, m.now_icon, x + 12, y + 34, fg, bg);
  text(d, f_.m15, x + 12, y + 94, fg, fit(f_.m15, m.now_title, 100), Align::LEFT, bg);
  text(d, f_.m12m, x + 12, y + 110, fg, fit(f_.m12m, m.now_body, 100), Align::LEFT, bg);
  text(d, f_.m10, x + 12, y + 123, alarm ? WH : tone(95), fit(f_.m10, m.now_detail, 100), Align::LEFT, bg);
}

// ---------------------------------------------------------------------------
void TilesSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute));
  const Color qb = tone(220);
  std::string q = fit(f_.m15, m.question, 350);
  int qw = measure(f_.m15, q) + 30;
  fill_round(d, 390 - qw, 30, qw, 34, 17, qb);
  text(d, f_.m15, 390 - qw + 15, 52, K, q, Align::LEFT, qb);

  fill_round(d, 10, 72, 380, 158, 20, tone(214));
  fill_round(d, 12, 74, 376, 154, 18, paper());
  AnswerLayout L = layout_answer(f_.answer, m, ANS_W, ANS_LINES);
  for (size_t i = 0; i < L.lines.size(); i++)
    text(d, f_.answer, ANS_X, ANS_Y + (int) i * ANS_PITCH, K, L.lines[i]);
  if (L.typing)
    fill_round(d, ANS_X + L.cursor_w + (L.cursor_w > 0 ? 4 : 0), ANS_Y + L.cursor_line * ANS_PITCH - 14, 8, 18, 2, K);

  // key facts as pills
  int x = 10;
  const Color pb = tone(212);
  for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
    const std::string &v = m.facts[i].first, &l = m.facts[i].second;
    const std::string ll = lower(l);
    const bool ic = contains_any(ll, T.kw_rain) || contains_any(ll, T.kw_precip);
    int w = measure(f_.m13, v) + 28 + (ic ? 18 : 0);
    int wl = l.empty() ? 0 : measure(f_.m11m, l) + 5;
    bool with_label = x + w + wl <= 390;
    if (with_label)
      w += wl;
    else if (x + w > 390)
      break;
    fill_round(d, x, 240, w, 30, 15, pb);
    if (ic)
      umbrella(d, x + 19, 253, K);
    int tx = x + 14 + (ic ? 18 : 0);
    text(d, f_.m13, tx, 260, K, v, Align::LEFT, pb);
    if (with_label)
      text(d, f_.m11m, tx + measure(f_.m13, v) + 5, 259, tone(95), l, Align::LEFT, pb);
    x += w + 8;
  }
  text(d, f_.m11m, 14, 293, grey(105), "Home Assistant");
  const int mw = measure(f_.m12, T.more) + 34;
  fill_round(d, 390 - mw, 276, mw, 22, 11, K);
  text(d, f_.m12, 390 - mw + 12, 291, WH, T.more, Align::LEFT, K);
  sym(d, Sym::DOWN, 372, 290, 7, WH);
}

// ---------------------------------------------------------------------------
void TilesSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const Color top = tone(54);
  fill_round(d, 10, 10, 380, 114, 20, top);
  if (fc_has_rain(m)) {
    cloud(d, 62, 56, 46, 248, 196, false);
    for (int x : {44, 58, 72, 86})
      thick_line(d, x, 84, x - 4, 96, 2.5f, WH, true);
  } else {
    sun(d, 62 + 0.944f * 46, 56 - 0.667f * 46, 0.36f * 46, 252, 215, false);
    cloud(d, 62, 56, 46, 248, 196, false);
  }
  std::string hi = deg(m.fc_hi);
  text(d, f_.fc_hi, 118, 84, WH, hi, Align::LEFT, top, -3);
  text(d, f_.fc_lo, 118 + measure(f_.fc_hi, hi, -3) + 6, 84, tone(185), "/ " + deg(m.fc_lo), Align::LEFT, top);
  text(d, f_.m13, 120, 108, WH, fit(f_.m13, std::string(T.tomorrow) + " · " + lower_first(m.fc_summary), 256), Align::LEFT, top);
  text(d, f_.m13, 376, 36, WH, cap_first(m.fc_day), Align::RIGHT, top);
  text(d, f_.m11m, 376, 52, tone(185), m.fc_date, Align::RIGHT, top);

  // chart tile
  const Color ct = tone(234);
  fill_round(d, 10, 132, 380, 110, 20, ct);
  float lo, hi_v;
  fc_range(m, lo, hi_v);
  const float lo_ref = std::floor(lo) - 2, span = std::max(7.0f, hi_v - lo_ref + 1);
  const int base = 222;
  auto X = [](float h) { return 28 + (h - 6) / 18 * 344; };
  auto Y = [&](float v) { return 214 - (v - lo_ref) / span * 62; };
  std::vector<std::pair<float, float>> pts;
  for (float h = 6; h <= 24.001f; h += 0.25f)
    pts.emplace_back(X(h), Y(fc_temp_at(m, h)));
  std::vector<std::pair<float, float>> area;
  area.emplace_back(X(6), (float) base);
  area.insert(area.end(), pts.begin(), pts.end());
  area.emplace_back(X(24), (float) base);
  fill_poly(d, area, tone(186));
  const float mx = std::max(0.7f, fc_rain_max(m));
  for (int i = 0; i < 18; i++) {
    float mm = m.fc_rain[i];
    if (std::isnan(mm) || mm < 0.05f)
      continue;
    int rh = std::max(4, (int) std::lround(mm / mx * 30));
    fill_round(d, (int) std::lround(X(6.0f + i)) - 4, base - rh, 8, rh, 4, tone(80));
  }
  polyline(d, pts, 2.6f, K);
  fill_circle(d, pts.front().first, pts.front().second, 1.3f, K);
  fill_circle(d, pts.back().first, pts.back().second, 1.3f, K);
  {
    const int pk = fc_peak_index(m);
    const float v = fc_temp_at(m, 6.0f + pk);
    float px = std::min(372.0f - 17, std::max(28.0f + 17, X(6.0f + pk))), py = Y(v);
    fill_round(d, (int) std::lround(px - 17), (int) std::lround(py - 27), 34, 19, 9, K);
    text(d, f_.m11, (int) std::lround(px), (int) std::lround(py - 13), WH, deg(v), Align::CENTER, K);
  }
  for (int h = 6; h <= 24; h += 6)
    text(d, f_.m10, (int) std::lround(X(h)), 236, tone(110), std::to_string(h) + ":00",
         h == 6 ? Align::LEFT : h == 24 ? Align::RIGHT : Align::CENTER, ct);

  // facts
  const std::pair<const char *, const std::string *> facts[3] = {
      {T.wind, &m.fc_wind}, {T.humidity, &m.fc_humidity}, {T.sunset, &m.fc_sunset}};
  const Color ft = tone(226);
  for (int i = 0; i < 3; i++) {
    const int x = 10 + i * 128;
    fill_round(d, x, 250, 124, 42, 14, ft);
    text(d, f_.m10, x + 12, 266, tone(100), facts[i].first, Align::LEFT, ft);
    text(d, f_.m14, x + 12, 284, K, fit(f_.m14, *facts[i].second, 104), Align::LEFT, ft);
  }
}

// ---------------------------------------------------------------------------
void TilesSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute));
  const float cx = 200, cy = 146, R = 98, th = 22;
  ring(d, cx, cy, R - th / 2, 2, K);
  arc(d, cx, cy, R - th / 2, th, TOP, TOP + timer_frac(m) * 2 * PI_F, K, true);
  text(d, f_.m15, 200, (int) cy - 34, K, fit(f_.m15, m.timer_name, 140), Align::CENTER);
  fixed_digits(d, f_.timer, 200, (int) cy + 16, K, mmss(m.timer_remaining));
  text(d, f_.m12, 200, (int) cy + 40, K, subst(T.ends_at, m.timer_end), Align::CENTER);
  pill_(d, 32, 262, 104, 30, "+1 min", false, f_.m13);
  pill_(d, 148, 262, 104, 30, cap_first(T.stop), true, f_.m13);
  pill_(d, 264, 262, 104, 30, cap_first(T.pause), false, f_.m13);
}

// ---------------------------------------------------------------------------
// LISTEN: one black tile, the voice as white rounded bars, a white pill with
// the elapsed time.
void TilesSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute) + " · " + m.room);
  fill_round(d, 10, 30, 380, 196, 20, K);
  text(d, f_.card, 30, 74, WH, T.listening, Align::LEFT, K, -1);
  fill_round(d, 300, 46, 74, 30, 15, paper());
  text(d, f_.m15, 337, 67, K, mmss(m.listen_seconds), Align::CENTER);
  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float l = std::min(1.0f, std::max(0.0f, m.mic_history[i]));
    int h = std::max(3, (int) std::lround(3 + l * 84));
    fill_round(d, 16 + (int) i * 4, (int) std::lround(150 - h / 2.0f), 3, h, 1, WH);
  }
  text(d, f_.m12m, 30, 214, WH, fit(f_.m12m, std::string(T.speak_naturally) + ". " + T.stop_when_quiet, 344), Align::LEFT, K);
  pill_(d, 32, 252, 104, 30, cap_first(T.finish), false, f_.m13);
  text(d, f_.m11m, 200, 271, grey(105), fit(f_.m11m, subst(T.wake_word_is, m.wake_word), 112), Align::CENTER);
  pill_(d, 264, 252, 104, 30, cap_first(T.cancel), true, f_.m13);
}

// THINK: the heard question as a bubble, three step tiles; the running one
// is black, finished ones get a check badge.
void TilesSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute) + " · " + m.room);
  const Color qb = tone(220);
  std::string q = fit(f_.m15, m.question, 350);
  int qw = measure(f_.m15, q) + 30;
  fill_round(d, 390 - qw, 30, qw, 34, 17, qb);
  text(d, f_.m15, 390 - qw + 15, 52, K, q, Align::LEFT, qb);
  text(d, f_.card, 16, 104, K, T.thinking, Align::LEFT, paper(), -1);
  for (int i = 0; i < 3; i++) {
    const int x = 10 + i * 128, y = 124, w = 124, h = 106;
    const bool done = i < m.pipeline_step, active = i == m.pipeline_step;
    const Color bg = active ? K : paper(), fg = active ? WH : K;
    if (active)
      fill_round(d, x, y, w, h, 18, K);
    else
      stroke_round(d, x, y, w, h, 18, K, done ? 2 : 1);
    const float bx = x + 26, by = y + 28;
    if (done) {
      fill_circle(d, bx, by, 12, K);
      check(d, bx - 6, by - 4, 12, 2.5f, WH);
    } else {
      if (active)
        fill_circle(d, bx, by, 12, WH);
      else
        ring(d, bx, by, 11.5f, 1, K);
      text(d, f_.m15, (int) bx, (int) by + 5, K, std::to_string(i + 1), Align::CENTER, active ? WH : paper());
    }
    auto lines = wrap_lines(f_.m13, T.steps[i], 100, 2);
    for (size_t j = 0; j < lines.size(); j++)
      text(d, f_.m13, x + 14, y + 70 + (int) j * 16, fg, lines[j], Align::LEFT, bg);
  }
  text(d, f_.m11m, 16, 271, grey(105), "Assist · Home Assistant");
  pill_(d, 264, 252, 104, 30, cap_first(T.cancel), true, f_.m13);
}

// DOORBELL: snapshot left; black info tile and pill buttons on the right.
void TilesSkin::doorbell_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  if (m.bell_image != nullptr && m.bell_image_ready) {
    d.start_clipping(0, 0, 264, H);
    d.image(0, 0, m.bell_image);
    d.end_clipping();
  } else {
    fill_rect(d, 0, 0, 264, H, tone(190));
    text(d, f_.m13, 132, 154, K, std::string(T.waiting_image) + "…", Align::CENTER, tone(190));
  }
  std::string tag = fit(f_.m11, std::string(T.camera) + " · " + m.bell_place, 220);
  int tw = measure(f_.m11, tag) + 24;
  fill_round(d, 10, 10, tw, 22, 11, K);
  text(d, f_.m11, 22, 25, WH, tag, Align::LEFT, K);

  fill_rect(d, 264, 0, 136, H, paper());
  fill_round(d, 272, 10, 118, 96, 18, K);
  text(d, f_.m11, 284, 32, WH, T.doorbell, Align::LEFT, K);
  text(d, f_.card, 283, 72, WH, m.bell_time, Align::LEFT, K, -1);
  text(d, f_.m12m, 284, 94, WH, fit(f_.m12m, m.bell_place, 96), Align::LEFT, K);
  auto cap = wrap_lines(f_.m15, m.bell_caption, 112, 3);
  for (size_t i = 0; i < cap.size(); i++)
    text(d, f_.m15, 276, 134 + (int) i * 19, K, cap[i]);
  if (!cap.empty())
    text(d, f_.m10, 276, 134 + (int) cap.size() * 19, grey(105), fit(f_.m10, T.ai_caption, 112));
  pill_(d, 272, 206, 118, 26, cap_first(T.open_door), true, f_.m13);
  pill_(d, 272, 238, 118, 26, cap_first(T.talk), false, f_.m13);
  pill_(d, 272, 270, 118, 26, cap_first(T.close_door), false, f_.m13);
}

}  // namespace note4_ui
}  // namespace esphome
