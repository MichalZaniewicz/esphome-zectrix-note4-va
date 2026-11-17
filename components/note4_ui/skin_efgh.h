// Small header-only helpers shared by the skins plakat, tarcza, piksel and cisza.
// Everything here only calls the drawing helpers declared in note4_ui.h.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {
namespace efgh {

static const float PI_F = 3.14159265f;
static const float TAU_F = 6.28318531f;

// The three pipeline steps shown on the THINK screen.
static const char *const STEPS[3] = {"Rozpoznano mowę", "Pytam Home Assistant", "Układam odpowiedź"};
static const char *const STEPS_SHORT[3] = {"mowa", "Home Assistant", "odpowiedź"};

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------------------------------------------------------- UTF-8 text
struct CasePair {
  const char *lo, *up;
};
static const CasePair PL_CASE[9] = {{"ą", "Ą"}, {"ć", "Ć"}, {"ę", "Ę"}, {"ł", "Ł"}, {"ń", "Ń"},
                                    {"ó", "Ó"}, {"ś", "Ś"}, {"ź", "Ź"}, {"ż", "Ż"}};

inline std::string recase_(const std::string &s, bool to_upper, size_t limit = std::string::npos) {
  std::string out;
  out.reserve(s.size());
  size_t i = 0, chars = 0;
  while (i < s.size()) {
    unsigned char c = s[i];
    bool apply = chars < limit;
    if (c < 0x80) {
      char ch = (char) c;
      if (apply) {
        if (to_upper && ch >= 'a' && ch <= 'z')
          ch = (char) (ch - 'a' + 'A');
        else if (!to_upper && ch >= 'A' && ch <= 'Z')
          ch = (char) (ch - 'A' + 'a');
      }
      out += ch;
      i++;
      chars++;
      continue;
    }
    size_t n = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
    std::string seq = s.substr(i, n);
    if (apply) {
      for (const auto &p : PL_CASE) {
        if (seq == (to_upper ? p.lo : p.up)) {
          seq = to_upper ? p.up : p.lo;
          break;
        }
      }
    }
    out += seq;
    i += n;
    chars++;
  }
  return out;
}
inline std::string upper(const std::string &s) { return recase_(s, true); }
inline std::string lower(const std::string &s) { return recase_(s, false); }
inline std::string cap_first(const std::string &s) { return recase_(s, true, 1); }

inline void pop_utf8(std::string &s) {
  while (!s.empty()) {
    unsigned char c = s.back();
    s.pop_back();
    if ((c & 0xC0) != 0x80)
      break;
  }
}
inline void rtrim(std::string &s) {
  while (!s.empty() && (s.back() == ' ' || s.back() == ',' || s.back() == '.'))
    s.pop_back();
}

// Shorten to max_w pixels, ending with an ellipsis.
inline std::string fit(Font *f, const std::string &s, int max_w) {
  if (f == nullptr || measure(f, s) <= max_w)
    return s;
  std::string t = s;
  while (!t.empty()) {
    pop_utf8(t);
    rtrim(t);
    if (measure(f, t + "…") <= max_w)
      return t + "…";
  }
  return "…";
}

// Wrap to at most n lines; the last one gets an ellipsis when text is cut.
inline std::vector<std::string> wrap_n(Font *f, const std::string &s, int max_w, int n) {
  std::vector<std::string> lines = wrap(f, s, max_w);
  if ((int) lines.size() > n && n > 0) {
    lines.resize(n);
    lines[n - 1] = fit(f, lines[n - 1] + "…", max_w);
  }
  return lines;
}

// First segment of "Mgła, potem słońce" -> "Mgła".
inline std::string first_part(const std::string &s) {
  size_t p = s.find(',');
  return p == std::string::npos ? s : s.substr(0, p);
}

inline int count_words(const std::string &s) {
  int n = 0;
  bool in = false;
  for (char c : s) {
    bool sp = (c == ' ' || c == '\n' || c == '\t');
    if (!sp && !in)
      n++;
    in = !sp;
  }
  return n;
}
inline std::string first_words(const std::string &s, int k) {
  int n = 0;
  bool in = false;
  for (size_t i = 0; i < s.size(); i++) {
    bool sp = (s[i] == ' ' || s[i] == '\n' || s[i] == '\t');
    if (!sp && !in) {
      if (n == k) {
        std::string t = s.substr(0, i);
        while (!t.empty() && t.back() == ' ')
          t.pop_back();
        return t;
      }
      n++;
    }
    in = !sp;
  }
  return s;
}

// -------------------------------------------------------------- numbers, time
inline std::string temp_str(float t, bool unit_c = false) {
  if (std::isnan(t))
    return unit_c ? "--°C" : "--°";
  int v = (int) lroundf(t);
  char b[16];
  if (unit_c)
    snprintf(b, sizeof(b), "%d°C", v);
  else
    snprintf(b, sizeof(b), "%d°", v);
  return b;
}
inline std::string strip0(const std::string &s) {
  if (s.size() > 1 && s[0] == '0' && s[1] != ':')
    return s.substr(1);
  return s;
}
// "17:45" -> 17.75, NAN when unparsable.
inline float parse_hm(const std::string &s) {
  int h = 0, mi = 0;
  if (sscanf(s.c_str(), "%d:%d", &h, &mi) == 2 && h >= 0 && h < 48 && mi >= 0 && mi < 60)
    return h + mi / 60.0f;
  return NAN;
}
inline float timer_frac(const Model &m) {
  if (m.timer_total <= 0)
    return 0;
  return clampf((float) m.timer_remaining / (float) m.timer_total, 0, 1);
}

// Polish weekday name -> short caps ("wtorek" -> "WT").
inline std::string short_day(const std::string &wd) {
  std::string l = lower(wd);
  static const char *const K[7][2] = {{"pon", "PN"}, {"wt", "WT"}, {"śr", "ŚR"}, {"czw", "CZ"},
                                      {"pi", "PT"},  {"sob", "SB"}, {"nie", "ND"}};
  for (const auto &k : K)
    if (l.compare(0, strlen(k[0]), k[0]) == 0)
      return k[1];
  return upper(wd.substr(0, 2));
}
// "17 listopada" -> day 17, month 11 (0 when unknown).
inline int day_of(const std::string &date) {
  int d = 0;
  sscanf(date.c_str(), "%d", &d);
  return d;
}
inline int month_of(const std::string &date) {
  static const char *const M[12] = {"sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paź", "lis", "gru"};
  size_t p = date.find(' ');
  if (p == std::string::npos)
    return 0;
  std::string mn = lower(date.substr(p + 1));
  for (int i = 0; i < 12; i++)
    if (mn.compare(0, strlen(M[i]), M[i]) == 0)
      return i + 1;
  return 0;
}
inline std::string ddmm(const std::string &date) {
  int d = day_of(date), mo = month_of(date);
  if (d <= 0)
    return date;
  char b[12];
  if (mo > 0)
    snprintf(b, sizeof(b), "%02d.%02d", d, mo);
  else
    snprintf(b, sizeof(b), "%d", d);
  return b;
}

// Forecast arrays cover hours 6..24 (19 entries).
inline float fc_temp_at(const Model &m, float h) {
  float x = clampf(h - 6.0f, 0, 18);
  int i = (int) floorf(x);
  if (i >= 18)
    return std::isnan(m.fc_temp[18]) ? 0 : m.fc_temp[18];
  float a = m.fc_temp[i], b = m.fc_temp[i + 1], u = x - i;
  if (std::isnan(a))
    a = std::isnan(b) ? 0 : b;
  if (std::isnan(b))
    b = a;
  return a + (b - a) * u;
}
inline float fc_rain_at(const Model &m, int h) {
  if (h < 6 || h > 24)
    return 0;
  float v = m.fc_rain[h - 6];
  return (std::isnan(v) || v < 0) ? 0 : v;
}
inline float fc_rain_total(const Model &m) {
  float s = 0;
  for (int h = 6; h <= 24; h++)
    s += fc_rain_at(m, h);
  return s;
}
inline void fc_temp_range(const Model &m, float &lo, float &hi) {
  lo = 1e9f;
  hi = -1e9f;
  for (float v : m.fc_temp) {
    if (std::isnan(v))
      continue;
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  if (lo > hi) {
    lo = 0;
    hi = 1;
  }
}
inline std::string mm_str(float mm, bool space = true) {
  char b[16];
  bool whole = mm >= 10 || fabsf(mm - roundf(mm)) < 0.05f;
  if (whole)
    snprintf(b, sizeof(b), "%d", (int) lroundf(mm));
  else
    snprintf(b, sizeof(b), "%.1f", mm);
  std::string out = b;
  if (!whole)
    for (auto &c : out)
      if (c == '.')
        c = ',';
  return out + (space ? " mm" : "mm");
}

// Fact value -> 0..1 for gauges ("15:00" -> 15/24, "6°" -> (6+10)/40, "92%" -> .92).
inline float fact_frac(const std::string &v) {
  float h = parse_hm(v);
  if (!std::isnan(h))
    return clampf(h / 24.0f, 0, 1);
  float x = 0;
  if (sscanf(v.c_str(), "%f", &x) != 1) {
    // Polish decimal comma
    std::string t = v;
    for (auto &c : t)
      if (c == ',')
        c = '.';
    if (sscanf(t.c_str(), "%f", &x) != 1)
      return 0.5f;
  }
  if (v.find("°") != std::string::npos)
    return clampf((x + 10) / 40.0f, 0, 1);
  if (v.find('%') != std::string::npos)
    return clampf(x / 100.0f, 0, 1);
  return clampf(x / 10.0f, 0.05f, 1);
}

inline std::string join_dot(const std::vector<std::string> &parts, const char *sep = " · ") {
  std::string out;
  for (const auto &p : parts) {
    if (p.empty())
      continue;
    if (!out.empty())
      out += sep;
    out += p;
  }
  return out;
}

// What the "Teraz ważne" card is about, as a short label.
inline const char *now_topic(const std::string &icon) {
  if (icon == "window")
    return "dom";
  if (icon == "wind")
    return "pogoda";
  if (icon == "plant")
    return "rośliny";
  if (icon == "vacuum")
    return "sprzątanie";
  if (icon == "school")
    return "szkoła";
  if (icon == "flood")
    return "zalanie";
  if (icon == "smoke")
    return "dym";
  return "dom";
}
// BRIEF: the sentence lives in now_body, falling back to now_title.
inline const std::string &brief_text(const Model &m) { return m.now_body.empty() ? m.now_title : m.now_body; }

// ---------------------------------------------------------------- drawing
inline void seg(Display &d, float cx, float cy, float a, float r1, float r2, float w, Color c, bool round = false) {
  thick_line(d, cx + cosf(a) * r1, cy + sinf(a) * r1, cx + cosf(a) * r2, cy + sinf(a) * r2, w, c, round);
}

// Rectangle w x h whose top-left corner is at (x, y), rotated by `rot` around it.
inline void rot_rect(Display &d, float x, float y, float w, float h, float rot, Color c) {
  float cs = cosf(rot), sn = sinf(rot);
  auto P = [&](float px, float py) { return std::make_pair(x + px * cs - py * sn, y + px * sn + py * cs); };
  fill_poly(d, {P(0, 0), P(w, 0), P(w, h), P(0, h)}, c);
}

// 1-bit checkerboard (crisp on partial refresh, unlike a tone).
inline void checker(Display &d, int x, int y, int w, int h, Color c = ink()) {
  for (int j = y; j < y + h; j++)
    for (int i = x; i < x + w; i++)
      if (((i + j) & 1) == 0)
        d.draw_pixel_at(i, j, c);
}

// "▲ label" style hint: symbol, 5 px gap, label at baseline y. Returns width.
inline int hint(Display &d, Font *f, Sym s, const std::string &label, int x, int y, Align a = Align::LEFT,
                Color c = ink(), Color bg = paper(), int size = 8) {
  int tw = size + 5 + measure(f, label);
  int x0 = a == Align::LEFT ? x : (a == Align::RIGHT ? x - tw : x - tw / 2);
  sym(d, s, x0, y - 1, size, c);
  text(d, f, x0 + size + 5, y, c, label, Align::LEFT, bg);
  return tw;
}

// Geometric icon for Model::now_icon, about 2r across.
inline void vector_icon(Display &d, const std::string &icon, float cx, float cy, float r, Color c) {
  if (icon == "window") {
    int s = (int) (r * 1.4f), x = (int) (cx - s / 2), y = (int) (cy - s / 2);
    stroke_rect(d, x, y, s, s, c, 2);
    vline(d, (int) cx, y, y + s, c, 2);
    hline(d, x, x + s, (int) cy, c, 2);
  } else if (icon == "wind") {
    thick_line(d, cx - r * .8f, cy - r * .45f, cx + r * .5f, cy - r * .45f, 2, c, true);
    thick_line(d, cx - r * .8f, cy, cx + r * .8f, cy, 2, c, true);
    thick_line(d, cx - r * .8f, cy + r * .45f, cx + r * .3f, cy + r * .45f, 2, c, true);
  } else if (icon == "plant") {
    thick_line(d, cx, cy + r * .4f, cx, cy - r * .5f, 2, c);
    fill_ellipse(d, cx - r * .35f, cy - r * .2f, r * .42f, r * .2f, -0.6f, c);
    fill_ellipse(d, cx + r * .35f, cy - r * .45f, r * .42f, r * .2f, 0.6f, c);
    fill_rect(d, (int) (cx - r * .4f), (int) (cy + r * .35f), (int) (r * .8f), (int) (r * .45f), c);
  } else if (icon == "vacuum") {
    ring(d, cx, cy, r * .8f, 2, c);
    fill_circle(d, cx, cy - r * .35f, r * .22f, c);
    arc(d, cx, cy, r * .5f, 1.5f, PI_F * 1.15f, PI_F * 1.85f, c);
  } else if (icon == "school") {
    int w = (int) (r * 1.5f), h = (int) (r * 1.1f);
    stroke_rect(d, (int) (cx - w / 2), (int) (cy - h / 2), w, h, c, 2);
    vline(d, (int) cx, (int) (cy - h / 2), (int) (cy + h / 2), c, 2);
  } else if (icon == "flood") {
    fill_circle(d, cx, cy + r * .25f, r * .5f, c);
    fill_poly(d, {{cx, cy - r * .85f}, {cx - r * .47f, cy + r * .1f}, {cx + r * .47f, cy + r * .1f}}, c);
  } else if (icon == "smoke") {
    fill_circle(d, cx - r * .4f, cy + r * .15f, r * .3f, c);
    fill_circle(d, cx, cy - r * .15f, r * .4f, c);
    fill_circle(d, cx + r * .42f, cy + r * .15f, r * .3f, c);
    fill_rect(d, (int) (cx - r * .4f), (int) (cy + r * .1f), (int) (r * .82f), (int) (r * .35f), c);
  } else if (icon == "brief" || icon.empty()) {
    fill_circle(d, cx, cy, r * .35f, c);
    for (int i = 0; i < 8; i++)
      seg(d, cx, cy, i * PI_F / 4, r * .55f, r * .85f, 1.6f, c);
  } else {
    fill_rect(d, (int) (cx - 1.5f), (int) (cy - r * .7f), 3, (int) (r * .9f), c);
    fill_circle(d, cx, cy + r * .55f, 1.8f, c);
  }
}

// Doorbell snapshot centred in the left `w` pixels. False when no image yet.
inline bool bell_image(Display &d, const Model &m, int w) {
  if (!m.bell_image_ready || m.bell_image == nullptr)
    return false;
  int iw = m.bell_image->get_width(), ih = m.bell_image->get_height();
  d.start_clipping(0, 0, w, H);
  d.image((w - iw) / 2, (H - ih) / 2, m.bell_image);
  d.end_clipping();
  return true;
}

// ---------------------------------------------------------------- answer typing
// The answer is wrapped as a whole (so lines never reflow while typing), then
// only the first `shown` words are kept.
struct AnswerLayout {
  std::vector<std::string> lines;  // visible text per line
  int cur = -1;                    // line holding the newest word, -1 when complete
  bool more = false;               // text was cut to max_lines
};
inline AnswerLayout layout_answer(Font *f, const std::string &s, int max_w, int max_lines, int shown) {
  AnswerLayout r;
  std::vector<std::string> all = wrap(f, s, max_w);
  if ((int) all.size() > max_lines && max_lines > 0) {
    r.more = true;
    all.resize(max_lines);
  }
  std::vector<int> wc;
  int total = 0;
  for (const auto &l : all) {
    wc.push_back(count_words(l));
    total += wc.back();
  }
  if (shown < 0 || shown >= count_words(s)) {  // complete (same count as the framework)
    r.lines = all;
    if (r.more && !r.lines.empty())
      r.lines.back() = fit(f, r.lines.back() + "…", max_w);
    return r;
  }
  int left = std::min(shown, total);  // words past the cut keep the cursor on the last line
  r.cur = 0;
  for (size_t i = 0; i < all.size() && left > 0; i++) {
    if (left >= wc[i]) {
      r.lines.push_back(all[i]);
      left -= wc[i];
    } else {
      r.lines.push_back(first_words(all[i], left));
      left = 0;
    }
    r.cur = (int) i;
  }
  while ((int) r.lines.size() <= r.cur)  // nothing typed yet: an empty first line
    r.lines.push_back("");
  return r;
}
// Live rectangle for typing: the current line plus the previous one (the
// cursor block jumps from the end of one line to the next).
inline std::vector<Rect> typing_regions(const AnswerLayout &a, int x, int w, int first_base, int pitch, int asc,
                                        int desc) {
  if (a.cur < 0)
    return {};
  int from = a.cur > 0 ? a.cur - 1 : 0;
  int top = first_base + from * pitch - asc - 2;
  int bottom = first_base + a.cur * pitch + desc + 2;
  return {Rect{x, top, w, bottom - top}};
}

}  // namespace efgh
}  // namespace note4_ui
}  // namespace esphome
