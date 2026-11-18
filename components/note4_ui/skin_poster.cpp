#include "skin_poster.h"
#include "skin_efgh.h"

namespace esphome {
namespace note4_ui {

using namespace efgh;

static const Color GREY_Q = tone(214);  // quarter circles, card fills

void PosterSkin::draw(Display &d, Screen s, const Model &m) {
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

std::vector<Rect> PosterSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{16, 26, 192, 192}};
      if (m.timer_active)
        r.push_back({200, 236, 200, 64});
      return r;
    }
    case Screen::LISTEN:
      return {{44, 100, 136, 48}, {12, 230, 376, 68}};
    case Screen::THINK:
      return {{28, 84, 168, 96}};
    case Screen::ANSWER: {
      AnswerLayout a = layout_answer(f_body_, m.answer, 368, this->answer_lines_(m), m.answer_words_shown);
      int asc = f_body_->get_baseline();
      return typing_regions(a, 10, 386, 104, 26, asc + 1, f_body_->get_height() - asc);
    }
    case Screen::TIMER:
      return {{20, 56, 200, 200}, {232, 116, 156, 52}};
    default:
      return {};
  }
}

int PosterSkin::answer_lines_(const Model &m) const { return m.facts.empty() ? 7 : 5; }

// ------------------------------------------------------------------ IDLE
void PosterSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  wedge(d, 400, 0, 92, PI_F / 2, PI_F, GREY_Q);

  // the big black circle with the clock
  fill_circle(d, 112, 122, 96, ink());
  text(d, f_small_, 112, 78, paper(), short_day(T, m.weekday) + " " + ddmm(m.date), Align::CENTER, ink(), 1.5f);
  fixed_digits(d, f_clock_, 112, 142, paper(), hhmm(m.hour, m.minute), ink());
  text(d, f_label_, 112, 174, paper(), fit(f_label_, upper(m.room), 150), Align::CENTER, ink(), 2);

  // weather now
  text(d, f_clock_, 224, 98, ink(), temp_str(m.temp_now));
  text(d, f_small_, 226, 120, ink(), fit(f_small_, m.condition, 158));
  text(d, f_small_, 226, 138, ink(110), temp_str(m.temp_hi) + " / " + temp_str(m.temp_lo));
  fill_rect(d, 226, 150, 158, 6, ink());

  // next event
  text(d, f_label_, 226, 176, ink(), upper(T.next), Align::LEFT, paper(), 1.5f);
  if (m.next_title.empty() && m.next_time.empty()) {
    text(d, f_head_, 226, 202, ink(), "—");
    text(d, f_bold_, 226, 221, ink(), fit(f_bold_, T.nothing_more_today, 158));
  } else {
    text(d, f_head_, 226, 202, ink(), m.next_time);
    int tx = 226 + measure(f_head_, m.next_time) + 8;
    if (!m.next_in.empty() && tx < 360)
      text(d, f_small_, tx, 202, ink(110), fit(f_small_, m.next_in, 384 - tx));
    text(d, f_bold_, 226, 221, ink(), fit(f_bold_, m.next_title, 158));
  }

  this->now_card_(d, m);

  // bottom right: black block, the running timer or the wake word
  fill_rect(d, 200, 236, 200, 64, ink());
  if (m.timer_active) {
    text(d, f_label_, 214, 254, paper(), fit(f_label_, upper(m.timer_name), 160), Align::LEFT, ink(), 1.5f);
    fixed_digits(d, f_num_, 214 + measure(f_num_, "00:00") / 2, 289, paper(), mmss(m.timer_remaining), ink());
    fill_rect(d, 320, 282, 64, 1, paper());
    fill_rect(d, 320, 280, (int) lroundf(64 * timer_frac(m)), 5, paper());
  } else {
    text(d, f_label_, 214, 254, paper(), upper(T.say), Align::LEFT, ink(), 1.5f);
    text(d, f_head_, 214, 287, paper(), fit(f_head_, subst(T.quoted, m.wake_word), 172), Align::LEFT, ink());
  }
}

// The "Now" card, bottom left (0,236 200x64).
void PosterSkin::now_card_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  bool alarm = m.now_level == NowLevel::ALARM;
  Color bg = alarm ? ink() : GREY_Q;
  Color fg = alarm ? paper() : ink();
  // an alarm is a black block; keep a white gap to the black timer block
  fill_rect(d, 0, 236, alarm ? 197 : 200, 64, bg);

  if (m.now_level == NowLevel::BRIEF) {
    text(d, f_label_, 14, 254, fg, fit(f_label_, upper(T.now) + " · " + upper(T.day), 170), Align::LEFT, bg, 1.5f);
    auto lines = wrap_n(f_small_, brief_text(m), 174, 2);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_small_, 14, 272 + (int) i * 15, fg, lines[i], Align::LEFT, bg);
    return;
  }

  std::string lab = upper(alarm ? T.alarm : T.now) + " · " + upper(now_topic(T, m.now_icon));
  text(d, f_label_, 14, 254, fg, fit(f_label_, lab, 170), Align::LEFT, bg, 1.5f);
  // the icon is a solid circle with the symbol cut out of it
  fill_circle(d, 32, 278, 16, fg);
  bool grade = m.now_icon == "school" && !m.now_detail.empty() && m.now_detail.size() <= 3;
  if (grade)
    text(d, f_head_, 32, 286, bg, m.now_detail, Align::CENTER, fg);
  else
    vector_icon(d, m.now_icon, 32, 278, 10, bg);
  text(d, f_bold_, 56, 274, fg, fit(f_bold_, m.now_title, 136), Align::LEFT, bg);
  std::string sub = grade ? m.now_body : join_dot({m.now_body, m.now_detail}, ", ");
  text(d, f_small_, 56, 289, fg, fit(f_small_, sub, 136), Align::LEFT, bg);
}

// ------------------------------------------------------------------ LISTEN
// The clock circle becomes the recorder: elapsed time inside, the voice as a
// black band of bars along the bottom edge.
void PosterSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  wedge(d, 400, 0, 92, PI_F / 2, PI_F, GREY_Q);
  fill_circle(d, 112, 122, 96, ink());
  text(d, f_label_, 112, 88, paper(), fit(f_label_, upper(T.listening), 150), Align::CENTER, ink(), 2);
  fixed_digits(d, f_timer_, 112, 142, paper(), mmss(m.listen_seconds), ink());
  fill_circle(d, 112, 166, 5, paper());

  auto sp = wrap_n(f_head_, std::string(T.speak_naturally) + ".", 158, 2);
  for (size_t i = 0; i < sp.size(); i++)
    text(d, f_head_, 226, 104 + (int) i * 24, ink(), sp[i]);
  fill_rect(d, 226, 142, 158, 6, ink());
  auto l = wrap_n(f_bold_, T.stop_when_quiet, 158, 2);
  for (size_t i = 0; i < l.size(); i++)
    text(d, f_bold_, 226, 166 + (int) i * 17, ink(), l[i]);
  hint(d, f_small_, Sym::DOT, T.finish, 226, 214);
  hint(d, f_small_, Sym::DOWN, T.cancel, 384, 214, Align::RIGHT);

  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float v = clampf(m.mic_history[i], 0, 1);
    int h = std::max(2, (int) lroundf(2 + v * 58));
    fill_rect(d, 16 + (int) i * 4, 264 - h / 2, 2, h, ink());
  }
}

// ------------------------------------------------------------------ THINK
// Inside the circle three dots for the pipeline steps; the heard question
// stands next to it as a headline.
void PosterSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  wedge(d, 400, 300, 80, PI_F, PI_F * 1.5f, GREY_Q);
  fill_circle(d, 112, 122, 96, ink());
  text(d, f_head_, 112, 106, paper(), T.thinking, Align::CENTER, ink());
  int st = clampi(m.pipeline_step, 0, 2);
  for (int i = 0; i < 3; i++) {
    float cx = 76 + i * 36, cy = 136;
    if (i < st)
      fill_circle(d, cx, cy, 8, paper());
    else if (i == st)
      fill_circle(d, cx, cy, 12, paper());
    else
      ring(d, cx, cy, 7, 2, paper());
  }
  text(d, f_small_, 112, 172, paper(), fit(f_small_, T.steps[st], 164), Align::CENTER, ink());

  text(d, f_label_, 226, 44, ink(), upper(T.heard), Align::LEFT, paper(), 1.5f);
  auto q = wrap_n(f_head_, m.question.empty() ? std::string("…") : m.question, 158, 5);
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_head_, 226, 76 + (int) i * 26, ink(), q[i]);
  fill_rect(d, 226, 76 + (int) (q.size() - 1) * 26 + 14, 96, 6, ink());
  hint(d, f_small_, Sym::DOWN, T.cancel, 226, 290);
}

// ------------------------------------------------------------------ ANSWER
void PosterSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  wedge(d, 400, 0, 70, PI_F / 2, PI_F, GREY_Q);
  text(d, f_label_, 16, 30, ink(), upper(T.question), Align::LEFT, paper(), 1.5f);
  text(d, f_head_, 16, 56, ink(), fit(f_head_, m.question, 320));
  fill_rect(d, 16, 68, 96, 6, ink());

  AnswerLayout a = layout_answer(f_body_, m.answer, 368, this->answer_lines_(m), m.answer_words_shown);
  for (size_t i = 0; i < a.lines.size(); i++)
    text(d, f_body_, 16, 104 + (int) i * 26, ink(), a.lines[i]);
  if (a.cur >= 0) {
    const std::string &l = a.lines[a.cur];
    int x = l.empty() ? 16 : std::min(16 + measure(f_body_, l) + 5, 382);
    fill_rect(d, x, 104 + a.cur * 26 - 15, 9, 18, ink());
  }

  // key facts as shapes: solid circle, grey square, ring
  for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
    int x0 = 16 + (int) i * 124, cx = x0 + 26, cy = 262;
    std::string v = fit(f_val_, m.facts[i].first, 46);
    if (i == 0) {
      fill_circle(d, cx, cy, 26, ink());
      text(d, f_val_, cx, cy + 5, paper(), v, Align::CENTER, ink());
    } else if (i == 1) {
      fill_rect(d, cx - 24, cy - 24, 48, 48, tone(170));
      text(d, f_val_, cx, cy + 5, ink(), v, Align::CENTER, tone(170));
    } else {
      ring(d, cx, cy, 24.5f, 3, ink());
      text(d, f_val_, cx, cy + 5, ink(), v, Align::CENTER);
    }
    auto lab = wrap_n(f_label_, upper(m.facts[i].second), 58, 2);
    int y0 = lab.size() > 1 ? cy - 2 : cy + 4;
    for (size_t j = 0; j < lab.size(); j++)
      text(d, f_label_, x0 + 60, y0 + (int) j * 12, ink(), lab[j], Align::LEFT, paper(), 0.5f);
  }
}

// ------------------------------------------------------------------ WEATHER
// Tomorrow: a huge temperature, a grey cloud, hours as black circles whose
// size is the temperature, grey bars for rain.
void PosterSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_label_, 16, 30, ink(), fit(f_label_, upper(T.tomorrow) + " · " + upper(m.fc_day), 368), Align::LEFT, paper(), 1.5f);
  std::string hi = temp_str(m.fc_hi);
  text(d, f_huge_, 8, 132, ink(), hi, Align::LEFT, paper(), -4);
  int w = measure(f_huge_, hi, -4);
  text(d, f_num_, 14 + w, 132, ink(130), temp_str(m.fc_lo));

  float total = fc_rain_total(m);
  if (total >= 0.2f) {
    Color g = tone(120);
    fill_circle(d, 290, 62, 22, g);
    fill_circle(d, 320, 50, 30, g);
    fill_circle(d, 352, 64, 20, g);
    fill_rect(d, 290, 62, 62, 22, g);
    for (int x : {298, 318, 338, 358})
      rot_rect(d, x, 96, 5, 18, 0.35f, ink());
  } else {
    fill_circle(d, 322, 64, 28, tone(120));
    for (int i = 0; i < 8; i++)
      seg(d, 322, 64, i * PI_F / 4, 36, 46, 4, ink());
  }
  text(d, f_bold_, 236, 142, ink(), fit(f_bold_, m.fc_summary, 148));
  text(d, f_small_, 236, 158, ink(110), fit(f_small_, m.fc_detail, 148));

  float lo, hi_t;
  fc_temp_range(m, lo, hi_t);
  float span = std::max(hi_t - lo, 1.0f);
  const int hrs[7] = {6, 9, 12, 15, 18, 21, 24};
  float sums[7] = {0};
  float maxsum = 0;
  for (int i = 0; i < 6; i++) {
    for (int q = hrs[i]; q < hrs[i] + 3; q++)
      sums[i] += fc_rain_at(m, q);
    maxsum = std::max(maxsum, sums[i]);
  }
  float k = maxsum > 0 ? std::min(16.0f, 36.0f / maxsum) : 16.0f;
  for (int i = 0; i < 7; i++) {
    int h = hrs[i], cx = 40 + i * 53, cy = 196;
    float v = fc_temp_at(m, h);
    float r = 7 + (v - lo) / span * 16;
    fill_circle(d, cx, cy, r, ink());
    std::string t = temp_str(v);
    if (r > 14)
      text(d, f_val_, cx, cy + 5, paper(), t, Align::CENTER, ink());
    else
      text(d, f_label_, cx, (int) (cy - r - 4), ink(), t, Align::CENTER);
    if (h < 24 && sums[i] > 0.05f) {
      int bh = std::max(2, (int) lroundf(sums[i] * k));
      fill_rect(d, cx - 8, 262 - bh, 16, bh, tone(140));
    }
    text(d, f_label_, cx, 278, ink(), std::to_string(h), Align::CENTER);
  }
  hline(d, 16, 384, 262, ink());

  std::vector<std::string> parts;
  if (!m.fc_wind.empty())
    parts.push_back(std::string(T.wind) + " " + m.fc_wind);
  if (!m.fc_humidity.empty())
    parts.push_back(std::string(T.humidity) + " " + m.fc_humidity);
  if (!m.fc_sunset.empty())
    parts.push_back(std::string(T.sunset) + " " + m.fc_sunset);
  text(d, f_small_, 16, 296, ink(100), fit(f_small_, join_dot(parts), 368));
}

// ------------------------------------------------------------------ TIMER
void PosterSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  wedge(d, 400, 300, 80, PI_F, PI_F * 1.5f, GREY_Q);
  text(d, f_label_, 16, 30, ink(), fit(f_label_, upper(m.timer_name), 200), Align::LEFT, paper(), 1.5f);
  const float cx = 120, cy = 156;
  ring(d, cx, cy, 96.5f, 3, ink());
  float fr = timer_frac(m);
  if (fr >= 0.999f)
    fill_circle(d, cx, cy, 98, ink());
  else if (fr > 0)
    wedge(d, cx, cy, 98, TOP, TOP + fr * TAU_F, ink());

  fixed_digits(d, f_timer_, 236 + measure(f_timer_, "00:00") / 2, 160, ink(), mmss(m.timer_remaining));
  text(d, f_bold_, 238, 186, ink(110), subst(T.of_total, mmss(m.timer_total)));
  if (!m.timer_end.empty())
    text(d, f_bold_, 238, 206, ink(), subst(T.ends_at, m.timer_end));
  hint(d, f_small_, Sym::UP, "+1 min", 16, 290);
  hint(d, f_small_, Sym::DOT, T.stop, 120, 290);
  hint(d, f_small_, Sym::DOWN, T.pause, 236, 290);
}

}  // namespace note4_ui
}  // namespace esphome
