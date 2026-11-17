// Skin "szwajcaria". Ported from note4-style.html (idleB, answerB, weatherB,
// timerB). LISTEN, THINK and DOORBELL follow the same grid: a 3 px rule under
// a huge word, three columns, the one black block marks what is happening.
#include "skin_szwajcaria.h"

#include <cmath>

#include "ui_util.h"

namespace esphome {
namespace note4_ui {

using namespace uu;

namespace {

const Color K = ink(0);
const Color WH = ink(255);

const char *const STEPS[3] = {"Rozpoznano mowę", "Pytam Home Assistant", "Układam odpowiedź"};

// Answer block (answerB).
const int ANS_X = 16, ANS_Y = 111, ANS_PITCH = 27, ANS_W = 368;

std::string total_label(int s) { return s % 60 == 0 ? std::to_string(s / 60) : mmss(s); }

}  // namespace

// ---------------------------------------------------------------------------
void SzwajcariaSkin::status_(Display &d, const Model &m, const std::string &left) {
  text(d, f_.i10b, 16, 15, K, fit(f_.i10b, upper(left), 230, 1), Align::LEFT, paper(), 1);
  std::string right = m.wifi ? "WI-FI" : "BEZ WI-FI";
  if (!m.ha)
    right += " · BEZ HA";
  right += "   ";
  if (m.charging)
    right += "ŁAD. ";
  right += m.battery >= 0 ? std::to_string(m.battery) + " %" : "– %";
  text(d, f_.i10b, 384, 15, K, right, Align::RIGHT, paper(), 1);
  hline(d, 16, 384, 22, K, 2);
}

void SzwajcariaSkin::draw(Display &d, Screen s, const Model &m) {
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

int SzwajcariaSkin::answer_lines_(const Model &m) const { return m.facts.empty() ? 7 : 5; }

std::vector<Rect> SzwajcariaSkin::live_regions(Screen s, const Model &m) const {
  switch (s) {
    case Screen::IDLE: {
      std::vector<Rect> r{{8, 30, 380, 106}};
      if (m.timer_active)
        r.push_back({262, 174, 124, 120});
      return r;
    }
    case Screen::LISTEN:
      return {{286, 32, 100, 58}, {12, 104, 376, 108}};
    case Screen::THINK:
      return {{8, 182, 384, 90}};
    case Screen::ANSWER: {
      AnswerLayout L = layout_answer(f_.i19, m, ANS_W, answer_lines_(m));
      return typing_rects(L, 10, 380, ANS_Y, ANS_PITCH, 21, 28);
    }
    case Screen::TIMER:
      return {{12, 76, 376, 112}, {12, 194, 376, 30}};
    default:
      return {};
  }
}

// ---------------------------------------------------------------------------
void SzwajcariaSkin::idle_(Display &d, const Model &m) {
  status_(d, m, weekday_short(m.weekday) + " " + ddmm(m.date) + " · " + m.room);
  text(d, f_.clock, 10, 130, K, hhmm(m.hour, m.minute), Align::LEFT, paper(), -3);
  std::string date = cap_first(m.weekday) + ", " + m.date;
  text(d, f_.i15s, 16, 156, K, date);
  int room = 368 - measure(f_.i15s, date) - 16;
  std::string wx = deg(m.temp_now) + " · " + m.condition;
  if (measure(f_.i15s, wx) > room)
    wx = deg(m.temp_now) + " · " + first_clause(m.condition);
  text(d, f_.i15s, 384, 156, K, fit(f_.i15s, wx, room), Align::RIGHT);
  hline(d, 16, 384, 168, K);
  vline(d, 132, 176, 292, K);
  vline(d, 256, 176, 292, K);

  // next
  text(d, f_.i9b, 16, 190, K, "NASTĘPNE", Align::LEFT, paper(), 1.2f);
  if (!m.next_time.empty() || !m.next_title.empty()) {
    text(d, f_.next, 15, 224, K, m.next_time, Align::LEFT, paper(), -1);
    text(d, f_.i13s, 16, 244, K, fit(f_.i13s, m.next_title, 110));
    text(d, f_.i12, 16, 261, grey(105), fit(f_.i12, m.next_in, 110));
  } else {
    text(d, f_.next, 15, 224, K, "–", Align::LEFT, paper(), -1);
    text(d, f_.i13s, 16, 244, K, "Wolne");
    text(d, f_.i12, 16, 261, grey(105), "nic w kalendarzu");
  }

  // now
  if (m.now_level == NowLevel::BRIEF) {
    text(d, f_.i9b, 142, 190, K, "DZIŚ", Align::LEFT, paper(), 1.2f);
    auto lines = wrap_lines(f_.i13s, brief_text(m), 108, 5);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_.i13s, 142, 210 + (int) i * 17, K, lines[i]);
  } else {
    const bool alarm = m.now_level == NowLevel::ALARM;
    const int x = alarm ? 144 : 142;
    const Color fg = alarm ? WH : K, bg = alarm ? K : paper();
    if (alarm)
      fill_rect(d, 136, 176, 116, 116, K);
    text(d, f_.i9b, x, 190, fg, alarm ? "ALARM" : "TERAZ", Align::LEFT, bg, 1.2f);
    auto title = wrap_lines(f_.title, m.now_title, alarm ? 102 : 110, 2);
    for (size_t i = 0; i < title.size(); i++)
      text(d, f_.title, x - 1, 220 + (int) i * 26, fg, title[i], Align::LEFT, bg, -0.5f);
    text(d, f_.i11b, x, 266, fg, fit(f_.i11b, m.now_body, alarm ? 102 : 110), Align::LEFT, bg);
    text(d, f_.i10s, x, 283, alarm ? WH : grey(105), fit(f_.i10s, m.now_detail, alarm ? 102 : 110), Align::LEFT, bg);
  }

  // timer block, or tomorrow when no timer runs
  if (m.timer_active) {
    fill_rect(d, 264, 176, 120, 116, K);
    text(d, f_.i9b, 274, 192, WH, "TIMER", Align::LEFT, K, 1.2f);
    text(d, f_.card, 273, 230, WH, mmss(m.timer_remaining), Align::LEFT, K, -1);
    text(d, f_.i13s, 274, 250, WH, fit(f_.i13s, m.timer_name, 100), Align::LEFT, K);
    fill_rect(d, 274, 268, 100, 1, WH);
    fill_rect(d, 274, 266, (int) std::lround(100 * timer_frac(m)), 5, WH);
  } else {
    text(d, f_.i9b, 266, 190, K, "JUTRO", Align::LEFT, paper(), 1.2f);
    text(d, f_.card, 265, 224, K, deg(m.fc_hi) + " / " + deg(m.fc_lo), Align::LEFT, paper(), -1);
    auto lines = wrap_lines(f_.i13s, m.fc_summary, 116, 2);
    for (size_t i = 0; i < lines.size(); i++)
      text(d, f_.i13s, 266, 244 + (int) i * 17, K, lines[i]);
    if (lines.size() < 2)
      text(d, f_.i12, 266, 261, grey(105), fit(f_.i12, m.fc_detail, 116));
  }
}

// ---------------------------------------------------------------------------
void SzwajcariaSkin::answer_(Display &d, const Model &m) {
  status_(d, m, hhmm(m.hour, m.minute) + " · " + m.room);
  text(d, f_.i9b, 16, 44, K, "PYTANIE", Align::LEFT, paper(), 1.2f);
  text(d, f_.question, 16, 67, K, fit(f_.question, m.question, 368, -0.3f), Align::LEFT, paper(), -0.3f);
  hline(d, 16, 384, 80, K, 3);
  AnswerLayout L = layout_answer(f_.i19, m, ANS_W, answer_lines_(m));
  for (size_t i = 0; i < L.lines.size(); i++)
    text(d, f_.i19, ANS_X, ANS_Y + (int) i * ANS_PITCH, K, L.lines[i]);
  if (L.typing)
    fill_rect(d, ANS_X + L.cursor_w + (L.cursor_w > 0 ? 4 : 0), ANS_Y + L.cursor_line * ANS_PITCH - 15, 9, 19, K);
  if (m.facts.empty())
    return;
  hline(d, 16, 384, 228, K);
  for (size_t i = 0; i < m.facts.size() && i < 3; i++) {
    const int x = 16 + (int) i * 124;
    if (i)
      vline(d, x - 10, 236, 292, K);
    text(d, f_.fact, x - 1, 266, K, fit(f_.fact, m.facts[i].first, 112, -1), Align::LEFT, paper(), -1);
    text(d, f_.i11s, x, 286, grey(105), fit(f_.i11s, m.facts[i].second, 112));
  }
}

// ---------------------------------------------------------------------------
void SzwajcariaSkin::weather_(Display &d, const Model &m) {
  status_(d, m, hhmm(m.hour, m.minute) + " · Pogoda");
  std::string hi = deg(m.fc_hi);
  text(d, f_.fc_hi, 8, 118, K, hi, Align::LEFT, paper(), -4);
  text(d, f_.fc_lo, 14 + measure(f_.fc_hi, hi, -4), 118, grey(135), deg(m.fc_lo));
  text(d, f_.h26, 236, 54, K, "Jutro", Align::LEFT, paper(), -0.5f);
  text(d, f_.i12s, 236, 72, grey(105), fit(f_.i12s, cap_first(m.fc_day) + ", " + m.fc_date, 148));
  text(d, f_.i14b, 236, 100, K, fit(f_.i14b, m.fc_summary, 148));
  std::string det = m.fc_detail;
  if (!m.fc_wind.empty())
    det += (det.empty() ? "wiatr " : " · wiatr ") + m.fc_wind;
  text(d, f_.i12s, 236, 118, grey(105), fit(f_.i12s, det, 148));
  hline(d, 16, 384, 134, K);

  // hourly bars: temperature up, rain down
  float lo, hi_v;
  fc_range(m, lo, hi_v);
  const float lo_ref = std::floor(lo) - 2, span = std::max(7.0f, hi_v - lo_ref + 1);
  const int base = 204, bw = 12;
  const float pitch = 368.0f / 18;
  const float mx = std::max(0.7f, fc_rain_max(m));
  const int peak = fc_peak_index(m) < 18 ? fc_peak_index(m) : 17;
  text(d, f_.i9b, 16, 150, grey(105), "°C");
  text(d, f_.i9b, 16, 226, grey(105), "mm");
  for (int i = 0; i < 18; i++) {
    const int h = 6 + i;
    const int x = (int) std::lround(16 + i * pitch + (pitch - bw) / 2);
    const float tv = fc_temp_at(m, (float) h);
    const int bh = std::max(1, (int) std::lround((tv - lo_ref) / span * 56));
    fill_rect(d, x, base - bh, bw, bh, K);
    float mm = m.fc_rain[i];
    if (!std::isnan(mm) && mm >= 0.05f)
      fill_rect(d, x, base + 4, bw, std::max(1, (int) std::lround(mm / mx * 30)), tone(120));
    if (h % 3 == 0)
      text(d, f_.i10b, x + bw / 2, 252, K, std::to_string(h), Align::CENTER);
    if (i == 0 || i == peak || i == 17)
      text(d, f_.i11b, x + bw / 2, base - bh - 5, K, deg(tv), Align::CENTER);
  }
  hline(d, 16, 384, base + 1, K);
  hline(d, 16, 384, 266, K);
  const std::pair<const char *, const std::string *> facts[3] = {
      {"Wilgotność", &m.fc_humidity}, {"Zachód", &m.fc_sunset}, {"Wiatr", &m.fc_wind}};
  for (int i = 0; i < 3; i++) {
    const int x = 16 + i * 124;
    const int lw = measure(f_.i11s, facts[i].first);
    text(d, f_.i11s, x, 287, grey(105), facts[i].first);
    text(d, f_.i12b, x + lw + 6, 287, K, fit(f_.i12b, *facts[i].second, 124 - lw - 14));
  }
}

// ---------------------------------------------------------------------------
void SzwajcariaSkin::timer_(Display &d, const Model &m) {
  status_(d, m, hhmm(m.hour, m.minute) + " · Timer");
  text(d, f_.h26, 16, 60, K, fit(f_.h26, m.timer_name, 260, -0.5f), Align::LEFT, paper(), -0.5f);
  text(d, f_.i13s, 384, 60, grey(105), "z " + mmss(m.timer_total), Align::RIGHT);
  fixed_digits(d, f_.clock, 200, 180, K, mmss(m.timer_remaining));
  stroke_rect(d, 16, 198, 368, 14, K);
  fill_rect(d, 16, 198, (int) std::lround(368 * timer_frac(m)), 14, K);
  for (int i = 0; i <= 10; i++) {
    int x = (int) std::lround(16 + i * 36.8f) - (i == 10 ? 1 : 0);
    vline(d, x, 214, i % 5 == 0 ? 222 : 218, K);
  }
  text(d, f_.i10b, 16, 234, K, "0");
  text(d, f_.i10b, 200, 234, K, dur_label(m.timer_total / 2), Align::CENTER);
  text(d, f_.i10b, 384, 234, K, total_label(m.timer_total), Align::RIGHT);
  hline(d, 16, 384, 248, K);
  text(d, f_.i11s, 16, 270, grey(105), "Koniec");
  text(d, f_.end, 15, 292, K, m.timer_end, Align::LEFT, paper(), -0.5f);
  hint(d, f_.i12b, Sym::DOWN, "Pauza", 384, 288, Align::RIGHT);
  hint(d, f_.i12b, Sym::DOT, "Stop", 300, 288, Align::RIGHT);
  hint(d, f_.i12b, Sym::UP, "+1 min", 232, 288, Align::RIGHT);
}

// ---------------------------------------------------------------------------
// LISTEN: the huge word, the elapsed time in the black block, the voice as
// black bars across the full width, then the idle screen's three columns.
void SzwajcariaSkin::listen_(Display &d, const Model &m) {
  status_(d, m, hhmm(m.hour, m.minute) + " · " + m.room);
  text(d, f_.big, 14, 82, K, "Słucham", Align::LEFT, paper(), -1.5f);
  fill_rect(d, 288, 34, 96, 54, K);
  text(d, f_.i9b, 296, 50, WH, "NAGRYWAM", Align::LEFT, K, 1.2f);
  text(d, f_.next, 295, 80, WH, mmss(m.listen_seconds), Align::LEFT, K, -1);
  hline(d, 16, 384, 96, K, 3);
  for (size_t i = 0; i < m.mic_history.size(); i++) {
    float l = std::min(1.0f, std::max(0.0f, m.mic_history[i]));
    int h = std::max(2, (int) std::lround(2 + l * 96));
    fill_rect(d, 16 + (int) i * 4, (int) std::lround(158 - h / 2.0f), 2, h, K);
  }
  hline(d, 16, 384, 216, K);
  vline(d, 132, 224, 292, K);
  vline(d, 256, 224, 292, K);
  text(d, f_.i9b, 16, 238, K, "MÓW NATURALNIE", Align::LEFT, paper(), 1.2f);
  text(d, f_.i13s, 16, 258, K, "Skończę, gdy");
  text(d, f_.i13s, 16, 275, K, "ucichniesz.");
  text(d, f_.i9b, 142, 238, K, "SŁOWO", Align::LEFT, paper(), 1.2f);
  text(d, f_.h26, 141, 268, K, fit(f_.h26, m.wake_word, 110, -0.5f), Align::LEFT, paper(), -0.5f);
  text(d, f_.i9b, 266, 238, K, "PRZYCISKI", Align::LEFT, paper(), 1.2f);
  hint(d, f_.i12b, Sym::DOT, "Zakończ", 266, 260, Align::LEFT);
  hint(d, f_.i12b, Sym::DOWN, "Anuluj", 266, 282, Align::LEFT);
}

// THINK: the question under the rule, the three pipeline steps as the grid's
// columns; the step in progress is the black block.
void SzwajcariaSkin::think_(Display &d, const Model &m) {
  status_(d, m, hhmm(m.hour, m.minute) + " · " + m.room);
  text(d, f_.big, 14, 82, K, "Chwila…", Align::LEFT, paper(), -1.5f);
  hline(d, 16, 384, 96, K, 3);
  text(d, f_.i9b, 16, 116, K, "USŁYSZAŁEM", Align::LEFT, paper(), 1.2f);
  auto q = wrap_lines(f_.question, "„" + m.question + "”", 368, 2);
  for (size_t i = 0; i < q.size(); i++)
    text(d, f_.question, 16, 140 + (int) i * 24, K, q[i], Align::LEFT, paper(), -0.3f);
  hline(d, 16, 384, 178, K);
  vline(d, 132, 186, 268, K);
  vline(d, 256, 186, 268, K);
  static const int BX[3] = {12, 138, 262};
  for (int i = 0; i < 3; i++) {
    const bool done = i < m.pipeline_step, active = i == m.pipeline_step;
    const int tx = BX[i] + 8;
    const Color fg = active ? WH : (done ? K : grey(105)), bg = active ? K : paper();
    if (active)
      fill_rect(d, BX[i], 186, 120, 82, K);
    text(d, f_.next, tx - 1, 222, fg, std::to_string(i + 1), Align::LEFT, bg, -1);
    if (done)
      check(d, tx + 26, 204, 14, 2.5f, K);
    auto lines = wrap_lines(f_.i13s, STEPS[i], 104, 2);
    for (size_t j = 0; j < lines.size(); j++)
      text(d, f_.i13s, tx, 244 + (int) j * 16, fg, lines[j], Align::LEFT, bg);
  }
  hint(d, f_.i12b, Sym::DOWN, "Anuluj", 384, 290, Align::RIGHT);
}

// DOORBELL: snapshot left, the black block on top of the panel with the time.
void SzwajcariaSkin::doorbell_(Display &d, const Model &m) {
  if (m.bell_image != nullptr && m.bell_image_ready) {
    d.start_clipping(0, 0, 264, H);
    d.image(0, 0, m.bell_image);
    d.end_clipping();
  } else {
    fill_rect(d, 0, 0, 264, H, tone(190));
    text(d, f_.i13s, 132, 154, K, "Czekam na obraz…", Align::CENTER, tone(190));
  }
  fill_rect(d, 264, 0, 136, H, paper());
  fill_rect(d, 264, 0, 136, 96, K);
  const int x = 276;
  text(d, f_.i9b, x, 22, WH, "DZWONEK", Align::LEFT, K, 1.2f);
  text(d, f_.next, x - 1, 62, WH, m.bell_time, Align::LEFT, K, -1);
  text(d, f_.i13s, x, 84, WH, fit(f_.i13s, m.bell_place, 112), Align::LEFT, K);
  auto cap = wrap_lines(f_.question, m.bell_caption, 112, 3);
  for (size_t i = 0; i < cap.size(); i++)
    text(d, f_.question, x, 124 + (int) i * 24, K, cap[i], Align::LEFT, paper(), -0.3f);
  if (!cap.empty())
    text(d, f_.i10s, x, 124 + (int) cap.size() * 24 - 4, grey(105), "opis AI ze zdjęcia");
  hline(d, x, 388, 200, K, 2);
  hint(d, f_.i12b, Sym::UP, "Otwórz", x, 226, Align::LEFT);
  hint(d, f_.i12b, Sym::DOT, "Mów", x, 252, Align::LEFT);
  hint(d, f_.i12b, Sym::DOWN, "Zamknij", x, 278, Align::LEFT);
}

}  // namespace note4_ui
}  // namespace esphome
