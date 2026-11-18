// Skin "paper". Ported from note4-style.html (idleA, answerA, weatherA,
// timerA) and note4-makiety.html (drawListen, drawTyping, drawDoorbell).
#include "skin_paper.h"

#include <cmath>

#include "ui_util.h"

namespace esphome {
namespace note4_ui {

using namespace uu;

namespace {

const Color K = ink(0);
const Color WH = ink(255);

// Answer block geometry (drawTyping).
const int ANS_X = 16, ANS_Y = 104, ANS_PITCH = 29, ANS_LINES = 6, ANS_W = 368;

}  // namespace

// ---------------------------------------------------------------------------
void PaperSkin::status_(Display &d, const Model &m, const std::string &left, bool inv) {
  const Color fg = inv ? WH : K, bg = inv ? K : paper();
  text(d, f_.l10b, 16, 15, fg, left, Align::LEFT, bg, 1.4f);
  // battery
  stroke_rect(d, 362, 6, 19, 10, fg);
  fill_rect(d, 381, 9, 2, 4, fg);
  if (m.battery >= 0) {
    int w = (int) std::lround(15 * std::min(100, m.battery) / 100.0f);
    fill_rect(d, 364, 8, w, 6, fg);
  }
  std::string pct = m.battery >= 0 ? std::to_string(m.battery) + "%" : "–";
  text(d, f_.l10b, 357, 15, fg, pct, Align::RIGHT, bg);
  if (m.charging)
    bolt(d, 357 - measure(f_.l10b, pct) - 9, 6, 9, fg);
  // wifi
  if (m.wifi) {
    for (float r : {3.5f, 7.0f, 10.5f})
      arc(d, 318, 16, r, 1.6f, -3.14159265f * 0.76f, -3.14159265f * 0.24f, fg);
  } else {
    thick_line(d, 310, 5, 326, 16, 1.6f, fg);
  }
  fill_circle(d, 318, 16, 1.4f, fg);
  // Home Assistant: house, door filled while connected
  polyline(d, {{294.5f, 11}, {300.5f, 5.5f}, {306.5f, 11}, {306.5f, 16.5f}, {294.5f, 16.5f}, {294.5f, 11}}, 1.3f, fg);
  if (m.ha)
    fill_rect(d, 299, 12, 2, 4, fg);
  hline(d, 0, W, 22, fg);
}

// ---------------------------------------------------------------------------
void PaperSkin::draw(Display &d, Screen s, const Model &m) {
  switch (s) {
    case Screen::IDLE:
      idle_(d, m);
      break;
    case Screen::LISTEN:
      listen_(d, m, false);
      break;
    case Screen::THINK:
      listen_(d, m, true);
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

std::vector<Rect> PaperSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{10, 30, 240, 84}};
      if (m.timer_active)
        r.push_back({264, 201, 121, 85});
      return r;
    }
    case Screen::LISTEN:
      return {{12, 124, 376, 90}, {328, 66, 60, 22}};
    case Screen::THINK:
      return {{12, 190, 376, 72}};
    case Screen::ANSWER: {
      AnswerLayout L = layout_answer(f_.answer, m, ANS_W, ANS_LINES);
      return typing_rects(L, 10, 380, ANS_Y, ANS_PITCH, 24, 32);
    }
    case Screen::TIMER:
      return {{102, 60, 196, 196}};
    default:
      return {};
  }
}

// ---------------------------------------------------------------------------
void PaperSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, upper(m.room), false);
  text(d, f_.clock, 13, 106, K, hhmm(m.hour, m.minute));
  text(d, f_.date, 16, 133, K, fit(f_.date, m.weekday + ", " + m.date, 236));
  dotted_vline(d, 256, 32, 142, K);

  // weather now
  IconStyle st;
  if (m.condition_icon == "partlycloudy" || m.condition_icon.empty()) {
    sun(d, 352, 48, 13, 245, 150, true);
    cloud(d, 318, 72, 36, 252, 176, true);
  } else {
    weather_icon(d, m.condition_icon, 318, 66, 36, st);
  }
  text(d, f_.temp, 270, 128, K, deg(m.temp_now));
  sym(d, Sym::UP, 316, 115, 7, K);
  text(d, f_.l11b, 327, 117, K, deg(m.temp_hi));
  sym(d, Sym::DOWN, 316, 129, 7, K);
  text(d, f_.l11b, 327, 131, K, deg(m.temp_lo));
  text(d, f_.l11, 270, 145, K, fit(f_.l11, m.condition, 384 - 270));

  // day timeline 06-22
  auto X = [](float h) { return (int) std::lround(16 + (h - 6) / 16 * 368); };
  const int ay = 180;
  const float nh = now_hours(m);
  fill_rect(d, 16, 162, X(nh) - 16, ay - 162, tone(214));
  for (const auto &e : m.events) {
    float a = std::max(6.0f, e.start_h), b = std::min(22.0f, e.end_h);
    if (b <= a)
      continue;
    int x0 = X(a), w = std::max(2, X(b) - x0);
    if (b - a > 1.0f) {  // long blocks lighter
      fill_rect(d, x0, 165, w, 11, tone(120));
      stroke_rect(d, x0, 165, w, 11, K);
    } else {
      fill_rect(d, x0, 165, w, 11, K);
    }
  }
  hline(d, 16, 384, ay, K);
  for (int h = 6; h <= 22; h++) {
    bool big = h % 3 == 0;
    vline(d, X(h), ay, ay + (big ? 5 : 3), K);
    if (big)
      text(d, f_.l10b, X(h), ay + 15, K, std::to_string(h), Align::CENTER);
  }
  const int nx = X(nh);
  fill_poly(d, {{nx - 5.0f, 156}, {nx + 5.0f, 156}, {(float) nx, 162}}, K);
  vline(d, nx, 160, ay, K);

  // cards
  const int cy = 203, ch = 81, cw = 117, xs[3] = {16, 141, 266};
  stroke_round(d, xs[0], cy, cw, ch, 6, K);
  text(d, f_.l9b, xs[0] + 10, cy + 16, K, upper(T.next), Align::LEFT, paper(), 1.3f);
  if (!m.next_title.empty() || !m.next_time.empty()) {
    text(d, f_.next, xs[0] + 10, cy + 43, K, m.next_time);
    text(d, f_.l13, xs[0] + 10, cy + 60, K, fit(f_.l13, m.next_title, cw - 20));
    text(d, f_.l11i, xs[0] + 10, cy + 74, K, fit(f_.l11i, m.next_in, cw - 20));
  } else {
    text(d, f_.next, xs[0] + 10, cy + 43, K, "–");
    text(d, f_.l13, xs[0] + 10, cy + 60, K, fit(f_.l13, T.nothing_more, cw - 20));
    text(d, f_.l11i, xs[0] + 10, cy + 74, K, fit(f_.l11i, T.in_calendar, cw - 20));
  }

  now_card_(d, m, xs[1], cy, cw, ch);

  if (m.timer_active) {
    fill_round(d, xs[2], cy, cw, ch, 6, K);
    text(d, f_.l9b, xs[2] + 10, cy + 16, WH, upper(T.timer), Align::LEFT, K, 1.3f);
    text(d, f_.card, xs[2] + 10, cy + 46, WH, mmss(m.timer_remaining), Align::LEFT, K);
    text(d, f_.l13, xs[2] + 10, cy + 63, WH, fit(f_.l13, m.timer_name, cw - 20), Align::LEFT, K);
    stroke_rect(d, xs[2] + 10, cy + 69, cw - 20, 6, WH);
    fill_rect(d, xs[2] + 11, cy + 70, (int) std::lround((cw - 22) * timer_frac(m)), 4, WH);
  } else {
    // No timer: tomorrow at a glance.
    stroke_round(d, xs[2], cy, cw, ch, 6, K);
    text(d, f_.l9b, xs[2] + 10, cy + 16, K, fit(f_.l9b, upper(T.tomorrow) + " · " + upper(weekday_short(T, m.fc_day)), cw - 12, 1.3f),
         Align::LEFT, paper(), 1.3f);
    text(d, f_.card, xs[2] + 10, cy + 46, K, deg(m.fc_hi) + " / " + deg(m.fc_lo));
    text(d, f_.l13, xs[2] + 10, cy + 63, K, fit(f_.l13, m.fc_summary, cw - 20));
    text(d, f_.l11i, xs[2] + 10, cy + 77, K, fit(f_.l11i, m.fc_detail, cw - 20));
  }

  text(d, f_.l10b, 200, 297, K, subst(T.say_wake, m.wake_word), Align::CENTER, paper(), 0.8f);
}

// The "Now" card: BRIEF as the mockup (italic sentence), TODO as a framed
// card like "Next", ALARM in negative.
void PaperSkin::now_card_(Display &d, const Model &m, int x, int y, int w, int h) {
  const Strings &T = *m.tr;
  const int tx = x + 10, tw = w - 20;
  if (m.now_level == NowLevel::BRIEF) {
    stroke_round(d, x, y, w, h, 6, K);
    text(d, f_.l9b, tx, y + 16, K, fit(f_.l9b, upper(T.now) + " · " + upper(T.day), w - 14, 1.3f), Align::LEFT, paper(), 1.3f);
    auto lines = wrap_lines(f_.brief, brief_text(m), w - 18, 4);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_.brief, tx, y + 33 + (int) i * 14, K, lines[i]);
    return;
  }
  const bool alarm = m.now_level == NowLevel::ALARM;
  const Color fg = alarm ? WH : K, bg = alarm ? K : paper();
  if (alarm)
    fill_round(d, x, y, w, h, 6, K);
  else
    stroke_round(d, x, y, w, h, 6, K);
  std::string label = upper(alarm ? T.alarm : T.now) + " · " + upper(now_kind(T, m.now_icon));
  text(d, f_.l9b, tx, y + 16, fg, fit(f_.l9b, label, w - 14, 1.3f), Align::LEFT, bg, 1.3f);
  auto title = wrap_lines(f_.title, m.now_title, tw, 2);
  if (title.size() <= 1) {
    if (!title.empty())
      text(d, f_.title, tx, y + 38, fg, title[0], Align::LEFT, bg);
    text(d, f_.l13, tx, y + 57, fg, fit(f_.l13, m.now_body, tw), Align::LEFT, bg);
    text(d, f_.l11i, tx, y + 73, fg, fit(f_.l11i, m.now_detail, tw), Align::LEFT, bg);
  } else {
    text(d, f_.title, tx, y + 36, fg, title[0], Align::LEFT, bg);
    text(d, f_.title, tx, y + 53, fg, title[1], Align::LEFT, bg);
    text(d, f_.l13, tx, y + 70, fg, fit(f_.l13, m.now_body, tw), Align::LEFT, bg);
  }
}

// ---------------------------------------------------------------------------
void PaperSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute) + " · " + upper(m.room), false);
  const Color band = tone(238);
  fill_rect(d, 0, 23, W, 48, band);
  hline(d, 0, W, 71, K);
  text(d, f_.l9b, 16, 40, K, upper(T.question), Align::LEFT, band, 1.6f);
  text(d, f_.question, 16, 61, K, fit(f_.question, subst(T.quoted, m.question), 368), Align::LEFT, band);

  AnswerLayout L = layout_answer(f_.answer, m, ANS_W, ANS_LINES);
  for (size_t i = 0; i < L.lines.size(); i++)
    text(d, f_.answer, ANS_X, ANS_Y + (int) i * ANS_PITCH, K, L.lines[i]);
  if (L.typing) {
    int cx = ANS_X + L.cursor_w + (L.cursor_w > 0 ? 5 : 0);
    fill_rect(d, cx, ANS_Y + L.cursor_line * ANS_PITCH - 17, 10, 21, K);
  }

  hline(d, 16, 384, 264, K);
  const std::string details = cap_first(T.details);
  const int kw = 21 + measure(f_.l12b, details);
  std::string foot;
  for (const auto &fct : m.facts) {
    std::string item = fct.first + (fct.second.empty() ? "" : " " + fct.second);
    std::string trial = foot.empty() ? item : foot + " · " + item;
    if (measure(f_.l11, trial) > 368 - kw - 12)
      break;
    foot = trial;
  }
  if (foot.empty())
    foot = T.answer_from_ha;
  text(d, f_.l11, 16, 288, K, fit(f_.l11, foot, 368 - kw - 12));
  keycap(d, f_.l12b, Sym::DOWN, details, 384, 289, Align::RIGHT);
}

// ---------------------------------------------------------------------------
void PaperSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute) + " · " + upper(T.weather), false);
  IconStyle st;
  st.cloud_top = 236;
  st.cloud_bottom = 120;
  if (fc_has_rain(m)) {
    weather_icon(d, "rainy", 56, 62, 46, st);
  } else {
    sun(d, 56 + 0.944f * 46, 62 - 0.667f * 46, 0.36f * 46, 245, 150, true);
    cloud(d, 56, 62, 46, 236, 120, true);
  }
  text(d, f_.l9b, 112, 40, K, fit(f_.l9b, upper(T.tomorrow) + " · " + upper(m.fc_day) + ", " + upper(m.fc_date), 272, 1.5f),
       Align::LEFT, paper(), 1.5f);
  std::string hi = deg(m.fc_hi);
  text(d, f_.fc_hi, 110, 90, K, hi);
  text(d, f_.fc_lo, 110 + measure(f_.fc_hi, hi) + 4, 90, K, "/ " + deg(m.fc_lo));
  std::string sum = m.fc_summary + (m.fc_detail.empty() ? "" : (m.fc_summary.empty() ? "" : ", ") + m.fc_detail);
  text(d, f_.l13, 112, 110, K, fit(f_.l13, sum, 272));

  // temperature curve
  float lo, hi_v;
  fc_range(m, lo, hi_v);
  const float lo_ref = std::floor(lo) - 2, span = std::max(7.0f, hi_v - lo_ref + 1);
  const int base = 196;
  auto X = [](float h) { return 40 + (h - 6) / 18 * 344; };
  auto TY = [&](float v) { return base - (v - lo_ref) / span * 62; };
  std::vector<std::pair<float, float>> pts;
  for (float h = 6; h <= 24.001f; h += 0.25f)
    pts.emplace_back(X(h), TY(fc_temp_at(m, h)));
  std::vector<std::pair<float, float>> area;
  area.emplace_back(X(6), (float) base);
  area.insert(area.end(), pts.begin(), pts.end());
  area.emplace_back(X(24), (float) base);
  fill_poly(d, area, tone(206));
  polyline(d, pts, 2.2f, K);
  hline(d, 40, 384, base, K);
  text(d, f_.l9b, 16, 140, K, "°C");
  text(d, f_.l9b, 16, 222, K, "mm");
  const int peak = fc_peak_index(m);
  struct Mark {
    int i;
    Align a;
  };
  std::vector<Mark> marks{{0, Align::LEFT}};
  if (peak > 0 && peak < 18)
    marks.push_back({peak, Align::CENTER});
  marks.push_back({18, Align::RIGHT});
  for (auto &mk : marks) {
    float h = 6.0f + mk.i, v = fc_temp_at(m, h);
    fill_circle(d, X(h), TY(v), 3.5f, K);
    int dx = mk.a == Align::LEFT ? 6 : mk.a == Align::RIGHT ? -6 : 0;
    text(d, f_.l12b, (int) std::lround(X(h)) + dx, (int) std::lround(TY(v) - 8), K, deg(v), mk.a);
  }

  // rain bars
  const int pb = 238;
  const float mx = std::max(0.7f, fc_rain_max(m));
  for (int i = 0; i < 18; i++) {
    float mm = m.fc_rain[i];
    if (std::isnan(mm) || mm < 0.05f)
      continue;
    int bh = std::max(2, (int) std::lround(mm / mx * 26));
    int x = (int) std::lround(X(6.0f + i)) - 6;
    fill_rect(d, x, pb - bh, 12, bh, tone(92));
    stroke_rect(d, x, pb - bh, 12, bh, K);
  }
  hline(d, 40, 384, pb, K);
  if (!std::isnan(m.fc_rain_start_h)) {
    int rx = (int) std::lround(X(m.fc_rain_start_h)) - 9;
    dotted_vline(d, rx, 122, pb, K);
    const std::string rain = upper(T.rain);
    if (rx + 5 + measure(f_.l9b, rain, 1.2f) <= 384)
      text(d, f_.l9b, rx + 5, 130, K, rain, Align::LEFT, paper(), 1.2f);
    else
      text(d, f_.l9b, rx - 4, 130, K, rain, Align::RIGHT, paper(), 1.2f);
  }
  for (int h = 6; h <= 24; h += 3) {
    vline(d, (int) std::lround(X(h)), pb, pb + 4, K);
    text(d, f_.l10b, (int) std::lround(X(h)), pb + 15, K, std::to_string(h), Align::CENTER);
  }

  hline(d, 16, 384, 262, K);
  const std::pair<std::string, const std::string *> facts[3] = {
      {upper(T.wind), &m.fc_wind}, {upper(T.humidity), &m.fc_humidity}, {upper(T.sunset_long), &m.fc_sunset}};
  for (int i = 0; i < 3; i++) {
    int x = 16 + i * 124;
    if (i)
      vline(d, x - 10, 268, 296, K);
    text(d, f_.l9b, x, 277, K, fit(f_.l9b, facts[i].first, 112, 1.2f), Align::LEFT, paper(), 1.2f);
    text(d, f_.l14b, x, 294, K, fit(f_.l14b, *facts[i].second, 112));
  }
}

// ---------------------------------------------------------------------------
void PaperSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  status_(d, m, hhmm(m.hour, m.minute) + " · " + upper(T.timer), false);
  const float cx = 200, cy = 158, R = 92, th = 14;
  const float fr = timer_frac(m);
  ring(d, cx, cy, R - th / 2, th, tone(196));
  arc(d, cx, cy, R - th / 2, th, TOP, TOP + fr * 6.2831853f, K);
  ring(d, cx, cy, R, 1, K);
  ring(d, cx, cy, R - th, 1, K);
  for (int i = 0; i < 10; i++) {
    float a = TOP + i * 3.14159265f / 5;
    thick_line(d, cx + std::cos(a) * (R + 3), cy + std::sin(a) * (R + 3), cx + std::cos(a) * (R + 8),
               cy + std::sin(a) * (R + 8), 2, K);
  }
  text(d, f_.l10b, 200, (int) cy - 32, K, fit(f_.l10b, upper(m.timer_name), 140, 2), Align::CENTER, paper(), 2);
  fixed_digits(d, f_.timer, 200, (int) cy + 17, K, mmss(m.timer_remaining));
  text(d, f_.l12, 200, (int) cy + 40, K, subst(T.of_total, mmss(m.timer_total)), Align::CENTER);
  text(d, f_.l9b, 16, 128, K, "START", Align::LEFT, paper(), 1.4f);
  text(d, f_.times, 16, 150, K, m.timer_start);
  text(d, f_.l9b, 384, 128, K, upper(T.end), Align::RIGHT, paper(), 1.4f);
  text(d, f_.times, 384, 150, K, m.timer_end, Align::RIGHT);
  keycap(d, f_.l12b, Sym::UP, "+1 min", 16, 289, Align::LEFT);
  keycap(d, f_.l12b, Sym::DOT, T.stop, 200, 289, Align::CENTER);
  keycap(d, f_.l12b, Sym::DOWN, T.pause, 384, 289, Align::RIGHT);
}

// ---------------------------------------------------------------------------
// LISTEN and THINK share the black screen (drawListen).
void PaperSkin::listen_(Display &d, const Model &m, bool thinking) {
  const Strings &T = *m.tr;
  fill_rect(d, 0, 0, W, H, K);
  status_(d, m, hhmm(m.hour, m.minute) + " · " + upper(m.room), true);
  if (thinking) {
    text(d, f_.big, 16, 88, WH, T.thinking, Align::LEFT, K);
    text(d, f_.l9b, 16, 126, WH, upper(T.heard), Align::LEFT, K, 1.6f);
    text(d, f_.answer, 16, 154, WH, fit(f_.answer, subst(T.quoted, m.question), 368), Align::LEFT, K);
    dotted_hline(d, 16, 384, 176, WH);
    for (int i = 0; i < 3; i++) {
      const int y = 206 + i * 24;
      const bool done = i < m.pipeline_step, active = i == m.pipeline_step;
      if (done) {
        fill_rect(d, 16, y - 11, 13, 13, WH);
        polyline(d, {{19, y - 5.0f}, {22, y - 2.0f}, {27, y - 8.0f}}, 2, K);
      } else {
        stroke_rect(d, 16, y - 11, 13, 13, WH);
        if (active)
          fill_rect(d, 20, y - 7, 6, 6, WH);
      }
      text(d, active ? f_.l13b : f_.l13, 38, y, WH, T.steps[i], Align::LEFT, K);
    }
    keycap(d, f_.l12b, Sym::DOWN, T.cancel, 384, 289, Align::RIGHT, WH, K);
    return;
  }
  text(d, f_.big, 16, 88, WH, T.listening, Align::LEFT, K);
  fill_circle(d, 336, 77, 4, WH);
  text(d, f_.l13b, 384, 82, WH, mmss(m.listen_seconds), Align::RIGHT, K);
  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float l = std::min(1.0f, std::max(0.0f, m.mic_history[i]));
    int h = std::max(2, (int) std::lround(2 + l * 78));
    fill_rect(d, 16 + (int) i * 4, (int) std::lround(168 - h / 2.0f), 2, h, WH);
  }
  text(d, f_.l13, 16, 248, WH, fit(f_.l13, std::string(T.speak_naturally) + ". " + T.stop_when_quiet, 368), Align::LEFT, K);
  keycap(d, f_.l12b, Sym::DOT, T.finish, 16, 289, Align::LEFT, WH, K);
  keycap(d, f_.l12b, Sym::DOWN, T.cancel, 384, 289, Align::RIGHT, WH, K);
}

// ---------------------------------------------------------------------------
void PaperSkin::doorbell_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  // snapshot, left 264 px
  if (m.bell_image != nullptr && m.bell_image_ready) {
    d.start_clipping(0, 0, 264, H);
    d.image(0, 0, m.bell_image);
    d.end_clipping();
  } else {
    fill_rect(d, 0, 0, 264, H, tone(190));
    text(d, f_.l13, 132, 154, K, std::string(T.waiting_image) + "…", Align::CENTER, tone(190));
  }
  std::string tag = upper(T.camera) + " · " + upper(m.bell_place);
  int tw = measure(f_.l9b, tag, 1.2f) + 20;
  fill_round(d, 10, 10, tw, 18, 9, K);
  text(d, f_.l9b, 10 + tw / 2, 23, WH, tag, Align::CENTER, K, 1.2f);

  fill_rect(d, 264, 0, 136, H, paper());
  fill_rect(d, 264, 0, 2, H, K);
  const int x = 280;
  text(d, f_.l9b, x, 34, K, upper(T.doorbell), Align::LEFT, paper(), 1.6f);
  text(d, f_.bell_time, x, 72, K, m.bell_time);
  text(d, f_.l11, x, 90, K, fit(f_.l11, m.bell_place + " · " + T.just_now, 106));
  hline(d, x, 386, 104, K);
  auto cap = wrap_lines(f_.caption, m.bell_caption, 106, 2);
  for (size_t i = 0; i < cap.size(); i++)
    text(d, f_.caption, x, 130 + (int) i * 24, K, cap[i]);
  if (!cap.empty())
    text(d, f_.l10i, x, 172, K, fit(f_.l10i, T.ai_caption, 106));
  hline(d, x, 386, 188, K);
  keycap(d, f_.l12b, Sym::UP, cap_first(T.open_door), x, 216, Align::LEFT);
  keycap(d, f_.l12b, Sym::DOT, cap_first(T.talk), x, 242, Align::LEFT);
  keycap(d, f_.l12b, Sym::DOWN, cap_first(T.close_door), x, 268, Align::LEFT);
}

}  // namespace note4_ui
}  // namespace esphome
