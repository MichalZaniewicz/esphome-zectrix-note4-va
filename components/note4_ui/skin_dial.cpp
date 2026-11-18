#include "skin_dial.h"
#include "skin_efgh.h"

namespace esphome {
namespace note4_ui {

using namespace efgh;

void DialSkin::draw(Display &d, Screen s, const Model &m) {
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
  }
}

std::vector<Rect> DialSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{8, 46, 210, 210}};
      if (m.timer_active)
        r.push_back({316, 28, 60, 60});
      return r;
    }
    case Screen::LISTEN:
      return {{76, 24, 248, 254}};
    case Screen::THINK:
      return {{12, 144, 376, 114}};
    case Screen::ANSWER: {
      AnswerLayout a = layout_answer(f_body_, m.answer, 368, this->answer_lines_(m), m.answer_words_shown);
      int asc = f_body_->get_baseline();
      return typing_regions(a, 10, 386, 102, 26, asc + 1, f_body_->get_height() - asc);
    }
    case Screen::TIMER:
      return {{76, 26, 248, 252}};
    default:
      return {};
  }
}

int DialSkin::answer_lines_(const Model &m) const { return m.facts.empty() ? 7 : 5; }

// Small sub-dial: grey track (or a 1 px ring when it is live), black arc,
// value in the middle, label under it.
void DialSkin::gauge_(Display &d, float cx, float cy, float r, float frac, const std::string &val,
                        const std::string &label, bool live) {
  if (live)
    ring(d, cx, cy, r, 1, ink());
  else
    ring(d, cx, cy, r, 5, tone(212));
  if (frac >= 0.999f)
    ring(d, cx, cy, r, 5, ink());
  else if (frac > 0.001f)
    arc(d, cx, cy, r, 5, TOP, TOP + frac * TAU_F, ink());
  if (!val.empty())
    text(d, f_num_, (int) cx, (int) cy + 5, ink(), val, Align::CENTER);
  if (!label.empty())
    text(d, f_label_, (int) cx, (int) (cy + r + 16), ink(), fit(f_label_, label, 76), Align::CENTER, paper(), 1.2f);
}

// ------------------------------------------------------------------ IDLE
void DialSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const float cx = 112, cy = 150, R = 100;
  ring(d, cx, cy, R, 2, ink());
  for (int i = 0; i < 60; i++) {
    bool major = i % 5 == 0;
    seg(d, cx, cy, TOP + i * PI_F / 30, R - 4, R - (major ? 14 : 8), major ? 2.5f : 1, ink());
  }
  const int nums[3][2] = {{12, 0}, {6, 6}, {9, 9}};
  for (const auto &n : nums) {
    float a = TOP + n[1] / 12.0f * TAU_F;
    text(d, f_num_, (int) (cx + cosf(a) * (R - 28)), (int) (cy + sinf(a) * (R - 28) + 6), ink(), std::to_string(n[0]),
         Align::CENTER);
  }
  // date window at 3 o'clock
  stroke_rect(d, 149, 139, 37, 22, ink());
  text(d, f_label_, 168, 155, ink(), short_day(T, m.weekday) + " " + std::to_string(day_of(m.date)), Align::CENTER);

  float hh = (m.hour % 12) + m.minute / 60.0f, mm = m.minute + m.second / 60.0f;
  seg(d, cx, cy, TOP + hh / 12 * TAU_F, -10, 52, 7, ink(), true);
  seg(d, cx, cy, TOP + mm / 60 * TAU_F, -14, 84, 4, ink(), true);
  fill_circle(d, cx, cy, 6, ink());
  fill_circle(d, cx, cy, 2.2f, paper());

  // sub-dials: weather now, timer (or battery)
  float tf = std::isnan(m.temp_now) ? 0 : clampf((m.temp_now + 10) / 40, 0, 1);
  this->gauge_(d, 262, 58, 26, tf, temp_str(m.temp_now), upper(first_part(m.condition)), false);
  if (m.timer_active) {
    this->gauge_(d, 346, 58, 26, timer_frac(m), strip0(mmss(m.timer_remaining)), upper(m.timer_name), true);
  } else if (m.battery >= 0) {
    this->gauge_(d, 346, 58, 26, m.battery / 100.0f, std::to_string(m.battery) + "%",
                 upper(m.charging ? T.charging : T.battery), false);
  }

  // next event
  text(d, f_label_, 236, 128, ink(), upper(T.next), Align::LEFT, paper(), 1.5f);
  if (m.next_time.empty() && m.next_title.empty()) {
    text(d, f_text_, 236, 154, ink(), fit(f_text_, T.nothing_more_today, 148));
  } else {
    text(d, f_big_, 236, 154, ink(), m.next_time);
    int tx = 236 + measure(f_big_, m.next_time) + 6;
    text(d, f_text_, tx, 154, ink(), fit(f_text_, m.next_title, 384 - tx));
  }
  dotted_hline(d, 236, 384, 170, ink());

  this->now_card_(d, m);

  dotted_hline(d, 236, 384, 258, ink());
  std::vector<std::string> sun;
  if (!m.fc_sunrise.empty())
    sun.push_back(std::string(T.sunrise) + " " + m.fc_sunrise);
  if (!m.fc_sunset.empty())
    sun.push_back(std::string(T.sunset) + " " + m.fc_sunset);
  text(d, f_small_, 236, 282, ink(105), fit(f_small_, join_dot(sun), 148));
}

// The "Now" card between the dotted rules (236..384, 176..258).
void DialSkin::now_card_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  if (m.now_level == NowLevel::BRIEF) {
    text(d, f_label_, 236, 190, ink(), fit(f_label_, upper(T.now) + " · " + upper(T.day), 148), Align::LEFT, paper(), 1.5f);
    auto l = wrap_n(f_text_, brief_text(m), 148, 3);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_text_, 236, 212 + (int) i * 17, ink(), l[i]);
    return;
  }
  bool alarm = m.now_level == NowLevel::ALARM;
  Color fg = alarm ? paper() : ink();
  Color bg = alarm ? ink() : paper();
  if (alarm) {
    fill_round(d, 230, 176, 158, 82, 6, ink());
    text(d, f_label_, 240, 192, fg, fit(f_label_, upper(T.alarm) + " · " + upper(now_topic(T, m.now_icon)), 140), Align::LEFT, bg,
         1.5f);
  } else {
    text(d, f_label_, 236, 190, fg, upper(T.now), Align::LEFT, bg, 1.5f);
  }
  float ix = alarm ? 258 : 254;
  ring(d, ix, 226, 16, 2, fg);
  vector_icon(d, m.now_icon, ix, 226, 9, fg);
  int tx = alarm ? 282 : 280, tw = 384 - tx;
  auto t = wrap_n(f_title_, m.now_title, tw, 2);
  int y = t.size() > 1 ? 216 : 226;
  for (size_t i = 0; i < t.size(); i++)
    text(d, f_title_, tx, y + (int) i * 20, fg, t[i], Align::LEFT, bg);
  std::string sub = join_dot({m.now_body, m.now_detail}, ", ");
  text(d, f_small_, tx, t.size() > 1 ? 252 : 244, alarm ? paper() : ink(105), fit(f_small_, sub, tw), Align::LEFT, bg);
}

// ------------------------------------------------------------------ LISTEN
// A dial whose rim is the voice: the last 92 levels as radial bars, newest at
// 12 o'clock, a seconds hand sweeping the elapsed time.
void DialSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const float cx = 200, cy = 150, R = 70;
  text(d, f_text_, 16, 30, ink(), fit(f_text_, std::string(T.speak_naturally) + ".", 150));
  auto hint_lines = wrap_n(f_small_, T.stop_when_quiet, 96, 2);
  for (size_t i = 0; i < hint_lines.size(); i++)
    text(d, f_small_, 16, 48 + (int) i * 16, ink(105), hint_lines[i]);
  fill_circle(d, 322, 26, 4, ink());
  text(d, f_label_, 384, 30, ink(), fit(f_label_, upper(T.mic), 54), Align::RIGHT, paper(), 1.5f);

  ring(d, cx, cy, R, 2, ink());
  for (int i = 0; i < 60; i++) {
    bool major = i % 5 == 0;
    seg(d, cx, cy, TOP + i * PI_F / 30, R - 4, R - (major ? 12 : 7), major ? 2 : 1, ink());
  }
  const size_t n = m.mic_history.size();
  for (size_t i = 0; i < n; i++) {
    float v = clampf(m.mic_history[i], 0, 1);
    float a = TOP + (float) (i + 1) / (float) n * TAU_F;
    seg(d, cx, cy, a, R + 5, R + 5 + 2 + v * 44, 2, ink());
  }
  float s = (m.listen_seconds % 60) / 60.0f;
  seg(d, cx, cy, TOP + s * TAU_F, -12, R - 16, 3, ink(), true);
  fill_circle(d, cx, cy, 6, ink());
  fill_circle(d, cx, cy, 2.2f, paper());

  fill_rect(d, (int) cx - 32, (int) cy - 36, 64, 15, paper());
  text(d, f_label_, (int) cx, (int) cy - 24, ink(), fit(f_label_, upper(T.listening), 60), Align::CENTER, paper(), 2);
  fill_rect(d, (int) cx - 34, (int) cy + 18, 68, 30, paper());
  stroke_rect(d, (int) cx - 34, (int) cy + 18, 68, 30, ink());
  fixed_digits(d, f_big_, (int) cx, (int) cy + 42, ink(), mmss(m.listen_seconds));

  hint(d, f_small_, Sym::DOT, T.finish, 16, 290);
  hint(d, f_small_, Sym::DOWN, T.cancel, 384, 290, Align::RIGHT);
}

// ------------------------------------------------------------------ THINK
// Three sub-dials, one per pipeline step: full = done, partial = working,
// empty = waiting.
void DialSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_label_, 16, 30, ink(110), upper(T.heard), Align::LEFT, paper(), 1.5f);
  auto q = wrap_n(f_head_, m.question.empty() ? std::string("…") : m.question, 368, 2);
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_head_, 16, 58 + (int) i * 30, ink(), q[i]);
  hline(d, 16, 384, 58 + (int) (q.size() - 1) * 30 + 14, ink());

  int st = clampi(m.pipeline_step, 0, 2);
  for (int i = 0; i < 3; i++) {
    float cx = 72 + i * 128, cy = 176;
    if (i < st) {
      this->gauge_(d, cx, cy, 26, 1, "", upper(T.steps_short[i]), true);
      polyline(d, {{cx - 8, cy + 1}, {cx - 2, cy + 7}, {cx + 9, cy - 6}}, 3, ink());
    } else {
      this->gauge_(d, cx, cy, 26, i == st ? 0.6f : 0, std::to_string(i + 1), upper(T.steps_short[i]), true);
    }
  }
  text(d, f_text_, 200, 250, ink(), std::string(T.steps[st]) + "…", Align::CENTER);
  hint(d, f_small_, Sym::DOWN, T.cancel, 384, 290, Align::RIGHT);
}

// ------------------------------------------------------------------ ANSWER
void DialSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_label_, 16, 30, ink(110), upper(T.question), Align::LEFT, paper(), 1.5f);
  text(d, f_head_, 16, 58, ink(), fit(f_head_, m.question, 368));
  hline(d, 16, 384, 72, ink());

  AnswerLayout a = layout_answer(f_body_, m.answer, 368, this->answer_lines_(m), m.answer_words_shown);
  for (size_t i = 0; i < a.lines.size(); i++)
    text(d, f_body_, 16, 102 + (int) i * 26, ink(), a.lines[i]);
  if (a.cur >= 0) {
    const std::string &l = a.lines[a.cur];
    int x = l.empty() ? 16 : std::min(16 + measure(f_body_, l) + 4, 384);
    fill_rect(d, x, 102 + a.cur * 26 - 15, 3, 19, ink());
  }

  for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
    float cx = 72 + i * 128;
    const std::string &v = m.facts[i].first;
    this->gauge_(d, cx, 244, 26, fact_frac(v), fit(f_num_, v, 44), "", false);
    text(d, f_small_, (int) cx, 290, ink(105), fit(f_small_, m.facts[i].second, 120), Align::CENTER);
  }
}

// ------------------------------------------------------------------ WEATHER
// Tomorrow as a 24 h dial: grey night, rain as radial strokes, temperatures
// inside, a pointer where the rain starts.
void DialSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const float cx = 138, cy = 158, R = 100;
  auto ang = [](float h) { return TOP + h / 24.0f * TAU_F; };
  float ss = parse_hm(m.fc_sunset), sr = parse_hm(m.fc_sunrise);
  if (!std::isnan(ss))
    arc(d, cx, cy, R - 6, 12, ang(ss), ang(24), tone(196));
  if (!std::isnan(sr))
    arc(d, cx, cy, R - 6, 12, ang(0), ang(sr), tone(196));
  ring(d, cx, cy, R, 1, ink());
  ring(d, cx, cy, R - 12, 1, ink());
  for (int h = 0; h < 24; h++)
    seg(d, cx, cy, ang(h), R, R + (h % 6 ? 4 : 8), h % 6 ? 1 : 2, ink());
  for (int h : {0, 6, 12, 18}) {
    float a = ang(h);
    text(d, f_num_, (int) (cx + cosf(a) * (R + 18)), (int) (cy + sinf(a) * (R + 18) + 5), ink(), std::to_string(h),
         Align::CENTER);
  }
  for (int h = 6; h < 24; h++) {
    float mm = fc_rain_at(m, h);
    if (mm > 0.02f)
      seg(d, cx, cy, ang(h + 0.5f), R - 19, R - 19 - clampf(mm * 32, 1, 28), 5, ink(), true);
  }
  for (int h : {6, 9, 12, 15, 18, 21}) {
    float a = ang(h);
    text(d, f_num_, (int) (cx + cosf(a) * 44), (int) (cy + sinf(a) * 44 + 5), ink(), temp_str(fc_temp_at(m, h)),
         Align::CENTER);
  }
  if (!std::isnan(m.fc_rain_start_h)) {
    float ra = ang(m.fc_rain_start_h);
    fill_poly(d,
              {{cx + cosf(ra) * (R + 2), cy + sinf(ra) * (R + 2)},
               {cx + cosf(ra - .07f) * (R + 13), cy + sinf(ra - .07f) * (R + 13)},
               {cx + cosf(ra + .07f) * (R + 13), cy + sinf(ra + .07f) * (R + 13)}},
              ink());
  }
  text(d, f_huge_, (int) cx, (int) cy + 6, ink(), temp_str(m.fc_hi), Align::CENTER);
  text(d, f_label_, (int) cx, (int) cy + 24, ink(115), "min " + temp_str(m.fc_lo), Align::CENTER);

  text(d, f_head_, 272, 58, ink(), fit(f_head_, m.fc_day, 120));
  text(d, f_small_, 272, 76, ink(105), fit(f_small_, lower(T.tomorrow) + ", " + m.fc_date, 120));
  text(d, f_text_, 272, 108, ink(), fit(f_text_, m.fc_summary, 120));
  text(d, f_small_, 272, 126, ink(105), fit(f_small_, m.fc_detail, 120));

  fill_rect(d, 272, 162, 16, 8, tone(196));
  text(d, f_small_, 296, 170, ink(), fit(f_small_, T.night, 96));
  thick_line(d, 280, 182, 280, 194, 5, ink(), true);
  text(d, f_small_, 296, 192, ink(), fit(f_small_, T.rain_per_hour, 96));
  text(d, f_num_, 280, 214, ink(), temp_str(fc_temp_at(m, 12)), Align::CENTER);
  text(d, f_small_, 296, 214, ink(), fit(f_small_, T.temperature, 96));
  fill_poly(d, {{274, 226}, {286, 226}, {280, 236}}, ink());
  text(d, f_small_, 296, 236, ink(), fit(f_small_, T.rain_starts, 96));

  std::vector<std::string> p;
  if (!m.fc_wind.empty())
    p.push_back(std::string(T.wind) + " " + m.fc_wind);
  if (!m.fc_humidity.empty())
    p.push_back(m.fc_humidity);
  text(d, f_small_, 272, 284, ink(105), fit(f_small_, join_dot(p), 120));
}

// ------------------------------------------------------------------ TIMER
void DialSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const float cx = 200, cy = 152, R = 112;
  ring(d, cx, cy, R, 2, ink());
  for (int i = 0; i < 60; i++) {
    bool major = i % 6 == 0;
    seg(d, cx, cy, TOP + i * PI_F / 30, R - 4, R - (major ? 16 : 9), major ? 2.5f : 1, ink());
  }
  // ten numerals spread over the full time, shown where they are whole minutes
  float total_min = m.timer_total / 60.0f;
  for (int i = 0; i < 10; i++) {
    float v = i * total_min / 10.0f;
    if (i > 0 && (total_min <= 0 || fabsf(v - roundf(v)) > 0.01f))
      continue;
    float a = TOP + i / 10.0f * TAU_F;
    text(d, f_num_, (int) (cx + cosf(a) * (R - 30)), (int) (cy + sinf(a) * (R - 30) + 5), ink(),
         std::to_string((int) lroundf(v)), Align::CENTER);
  }
  float fr = timer_frac(m);
  if (fr >= 0.999f)
    ring(d, cx, cy, R + 6, 6, ink());
  else if (fr > 0.001f)
    arc(d, cx, cy, R + 6, 6, TOP, TOP + fr * TAU_F, ink());
  seg(d, cx, cy, TOP + fr * TAU_F, -16, R - 22, 3, ink(), true);
  fill_circle(d, cx, cy, 7, ink());
  fill_circle(d, cx, cy, 2.6f, paper());

  text(d, f_label_, (int) cx, (int) cy - 34, ink(), fit(f_label_, upper(m.timer_name), 120), Align::CENTER, paper(), 2);
  fill_rect(d, 155, 181, 90, 30, paper());
  stroke_rect(d, 154, 180, 92, 31, ink());
  fixed_digits(d, f_big_, (int) cx, (int) cy + 52, ink(), mmss(m.timer_remaining));

  hint(d, f_small_, Sym::UP, "+1 min", 16, 290);
  hint(d, f_small_, Sym::DOT, T.stop, 200, 290, Align::CENTER);
  hint(d, f_small_, Sym::DOWN, T.pause, 384, 290, Align::RIGHT);
}

}  // namespace note4_ui
}  // namespace esphome
