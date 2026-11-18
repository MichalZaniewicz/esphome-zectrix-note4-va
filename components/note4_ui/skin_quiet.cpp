#include "skin_quiet.h"
#include "skin_efgh.h"

namespace esphome {
namespace note4_ui {

using namespace efgh;

namespace {
const int ANSWER_LINES = 5;
}  // namespace

void QuietSkin::draw(Display &d, Screen s, const Model &m) {
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

std::vector<Rect> QuietSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{8, 52, 384, 128}};
      if (m.timer_active)
        r.push_back({260, 226, 126, 32});
      return r;
    }
    case Screen::LISTEN:
      return {{12, 90, 376, 120}, {150, 224, 100, 22}};
    case Screen::THINK:
      return {{12, 214, 376, 44}};
    case Screen::ANSWER: {
      AnswerLayout a = layout_answer(f_body_, m.answer, 368, ANSWER_LINES, m.answer_words_shown);
      int asc = f_body_->get_baseline();
      return typing_regions(a, 10, 384, 102, 34, asc + 1, f_body_->get_height() - asc);
    }
    case Screen::TIMER:
      return {{16, 76, 368, 124}, {12, 216, 376, 10}};
    default:
      return {};
  }
}

// ------------------------------------------------------------------ IDLE
void QuietSkin::idle_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_small_, 16, 36, ink(105), fit(f_small_, cap_first(m.weekday) + ", " + m.date, 200));
  std::string w = temp_str(m.temp_now);
  if (!m.condition.empty())
    w += " · " + lower(first_part(m.condition));
  text(d, f_small_, 384, 36, ink(105), fit(f_small_, w, 160), Align::RIGHT);

  fixed_digits(d, f_huge_, 12 + measure(f_huge_, "00:00") / 2, 170, ink(), hhmm(m.hour, m.minute));
  fill_rect(d, 16, 192, 368, 1, tone(185));

  // three quiet columns: next, now, timer
  text(d, f_tiny_, 16, 220, ink(110), fit(f_tiny_, T.next, 116));
  if (m.next_time.empty() && m.next_title.empty()) {
    text(d, f_body_, 16, 252, ink(), "—");
    text(d, f_small_, 16, 274, ink(110), fit(f_small_, lower(T.nothing_more_today), 116));
  } else {
    text(d, f_body_, 16, 252, ink(), fit(f_body_, m.next_time, 116));
    text(d, f_small_, 16, 274, ink(110), fit(f_small_, m.next_title, 116));
  }

  this->now_col_(d, m);

  const int x = 264;
  if (m.timer_active) {
    text(d, f_tiny_, x, 220, ink(110), fit(f_tiny_, cap_first(m.timer_name), 116));
    text(d, f_body_, x, 252, ink(), mmss(m.timer_remaining));
    if (!m.timer_end.empty())
      text(d, f_small_, x, 274, ink(110), subst(T.until, m.timer_end));
  } else {
    text(d, f_tiny_, x, 220, ink(110), fit(f_tiny_, T.say, 116));
    text(d, f_body_, x, 252, ink(), fit(f_body_, subst(T.quoted, m.wake_word), 116));
    text(d, f_small_, x, 274, ink(110), fit(f_small_, T.and_ask, 116));
  }
}

// Middle column. An alarm is the only black block quiet ever draws.
void QuietSkin::now_col_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  const int x = 140, w = 116;
  if (m.now_level == NowLevel::BRIEF) {
    text(d, f_tiny_, x, 220, ink(110), T.today);
    auto l = wrap_n(f_small_, brief_text(m), w, 4);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_small_, x, 238 + (int) i * 16, ink(), l[i]);
    return;
  }
  bool alarm = m.now_level == NowLevel::ALARM;
  Color bg = alarm ? ink() : paper();
  Color fg = alarm ? paper() : ink();
  Color dim = alarm ? paper() : ink(110);
  int tx = x, tw = w;
  if (alarm) {
    fill_rect(d, 134, 200, 124, 98, ink());
    tx = 142;
    tw = 108;
  }
  text(d, f_tiny_, tx, 220, dim, alarm ? T.alarm : T.now, Align::LEFT, bg);
  if (measure(f_body_, m.now_title) <= tw) {
    text(d, f_body_, tx, 252, fg, m.now_title, Align::LEFT, bg);
    std::string sub = m.now_body.empty() ? m.now_detail : m.now_body;
    text(d, f_small_, tx, 274, dim, fit(f_small_, sub, tw), Align::LEFT, bg);
  } else {
    auto l = wrap_n(f_body_, m.now_title, tw, 2);
    for (size_t i = 0; i < l.size(); i++)
      text(d, f_body_, tx, 248 + (int) i * 28, fg, l[i], Align::LEFT, bg);
  }
}

// ------------------------------------------------------------------ LISTEN
// Just the voice: a thin waveform in the middle, the rest whispered.
void QuietSkin::listen_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_italic_, 200, 64, ink(105), T.listening, Align::CENTER);
  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float v = clampf(m.mic_history[i], 0, 1);
    int h = std::max(2, (int) lroundf(2 + v * 110));
    fill_rect(d, 16 + (int) i * 4, 150 - h / 2, 2, h, ink());
  }
  text(d, f_small_, 200, 240, ink(110), mmss(m.listen_seconds), Align::CENTER);
  text(d, f_tiny_, 200, 266, ink(120), fit(f_tiny_, std::string(T.speak_naturally) + ". " + T.stop_when_quiet, 368),
       Align::CENTER);
  text(d, f_small_, 200, 288, ink(120), std::string(T.finish) + "          " + T.cancel, Align::CENTER);
}

// ------------------------------------------------------------------ THINK
// The heard question is the one big thing; a hairline shows how far it got.
void QuietSkin::think_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_italic_, 200, 64, ink(105), T.heard, Align::CENTER);
  auto q = wrap_n(f_body_, m.question.empty() ? std::string("…") : m.question, 340, 3);
  int y0 = 150 - (int) (q.size() - 1) * 17;
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_body_, 200, y0 + (int) i * 34, ink(), q[i], Align::CENTER);

  int st = clampi(m.pipeline_step, 0, 2);
  hline(d, 16, 384, 222, ink());
  fill_rect(d, 16, 221, (int) lroundf(368 * (st + 0.5f) / 3.0f), 3, ink());
  text(d, f_small_, 200, 250, ink(110), std::string(T.steps[st]) + "…", Align::CENTER);
  text(d, f_small_, 200, 288, ink(120), T.cancel, Align::CENTER);
}

// ------------------------------------------------------------------ ANSWER
void QuietSkin::answer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  if (!m.question.empty())
    text(d, f_italic_, 16, 52, ink(105), fit(f_italic_, subst(T.quoted, m.question), 368));
  AnswerLayout a = layout_answer(f_body_, m.answer, 368, ANSWER_LINES, m.answer_words_shown);
  for (size_t i = 0; i < a.lines.size(); i++)
    text(d, f_body_, 16, 102 + (int) i * 34, ink(), a.lines[i]);
  if (a.cur >= 0) {
    const std::string &l = a.lines[a.cur];
    int x = l.empty() ? 16 : std::min(16 + measure(f_body_, l) + 4, 386);
    fill_rect(d, x, 102 + a.cur * 34 - 22, 2, 27, ink());
  }
  fill_rect(d, 16, 258, 368, 1, tone(185));
  std::vector<std::string> facts;
  for (const auto &f : m.facts)
    facts.push_back(f.second.empty() ? f.first : f.first + " " + f.second);
  text(d, f_tiny_, 16, 284, ink(115), fit(f_tiny_, join_dot(facts), 368));
}

// ------------------------------------------------------------------ WEATHER
void QuietSkin::weather_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_small_, 16, 36, ink(105), fit(f_small_, std::string(T.tomorrow) + ", " + lower_name(T, m.fc_day), 368));
  std::string hi = temp_str(m.fc_hi), lo = "/ " + temp_str(m.fc_lo);
  text(d, f_huge_, 10, 158, ink(), hi);
  int w = measure(f_huge_, hi);
  text(d, f_italic_, 16 + w, 158, ink(130), lo);
  int sx = std::max(236, 16 + w + measure(f_italic_, lo) + 16);
  text(d, f_body_, sx, 104, ink(), fit(f_body_, m.fc_summary, 384 - sx));
  text(d, f_small_, sx, 126, ink(110), fit(f_small_, m.fc_detail, 384 - sx));

  float tlo, thi;
  fc_temp_range(m, tlo, thi);
  float span = std::max(thi - tlo, 5.0f);
  auto X = [](float h) { return 16 + (h - 6) / 18.0f * 368; };
  auto Y = [&](float v) { return 230 - (v - tlo) / span * 36; };
  std::vector<std::pair<float, float>> pts, poly;
  for (int k = 0; k <= 72; k++) {
    float h = 6 + k * 0.25f;
    pts.push_back({X(h), Y(fc_temp_at(m, h))});
  }
  poly.push_back({X(6), 236});
  poly.insert(poly.end(), pts.begin(), pts.end());
  poly.push_back({X(24), 236});
  fill_poly(d, poly, tone(236));
  polyline(d, pts, 1.5f, ink());
  for (int h = 6; h <= 24; h++) {
    int bh = std::min(30, (int) lroundf(fc_rain_at(m, h) * 24));
    if (bh > 0)
      fill_rect(d, (int) lroundf(X(h)) - 1, 262 - bh, 2, bh, tone(140));
  }
  text(d, f_tiny_, (int) X(6), 278, ink(120), "6:00");
  text(d, f_tiny_, (int) X(12), 278, ink(120), "12:00", Align::CENTER);
  text(d, f_tiny_, (int) X(18), 278, ink(120), "18:00", Align::CENTER);
  text(d, f_tiny_, (int) X(24), 278, ink(120), "24:00", Align::RIGHT);

  std::vector<std::string> p;
  if (!m.fc_wind.empty())
    p.push_back(lower(T.wind) + " " + m.fc_wind);
  if (!m.fc_humidity.empty())
    p.push_back(lower(T.humidity) + " " + m.fc_humidity);
  if (!m.fc_sunset.empty())
    p.push_back(lower(T.sunset) + " " + m.fc_sunset);
  text(d, f_tiny_, 16, 296, ink(115), fit(f_tiny_, join_dot(p), 368));
}

// ------------------------------------------------------------------ TIMER
void QuietSkin::timer_(Display &d, const Model &m) {
  const Strings &T = *m.tr;
  text(d, f_italic_, 200, 64, ink(105), fit(f_italic_, cap_first(m.timer_name), 360), Align::CENTER);
  fixed_digits(d, f_huge_, 200, 192, ink(), strip0(mmss(m.timer_remaining)));
  hline(d, 16, 384, 222, ink());
  fill_rect(d, 16, 221, (int) lroundf(368 * timer_frac(m)), 3, ink());
  if (!m.timer_end.empty())
    text(d, f_small_, 200, 250, ink(110), subst(T.until, m.timer_end), Align::CENTER);
  text(d, f_small_, 200, 288, ink(120), std::string("+1 min       ") + T.stop + "       " + T.pause, Align::CENTER);
}

}  // namespace note4_ui
}  // namespace esphome
