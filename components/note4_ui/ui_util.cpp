#include "ui_util.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace esphome {
namespace note4_ui {
namespace uu {

// ---------------------------------------------------------------------------
// UTF-8

static uint32_t decode(const std::string &s, size_t &i) {
  const uint8_t c0 = (uint8_t) s[i];
  int len = c0 < 0x80 ? 1 : (c0 >> 5) == 0x6 ? 2 : (c0 >> 4) == 0xE ? 3 : (c0 >> 3) == 0x1E ? 4 : 0;
  if (len == 0 || i + len > s.size()) {
    i++;
    return '?';
  }
  uint32_t cp = len == 1 ? c0 : len == 2 ? (c0 & 0x1F) : len == 3 ? (c0 & 0x0F) : (c0 & 0x07);
  for (int k = 1; k < len; k++)
    cp = (cp << 6) | ((uint8_t) s[i + k] & 0x3F);
  i += len;
  return cp;
}

static void encode(uint32_t cp, std::string &out) {
  if (cp < 0x80) {
    out += (char) cp;
  } else if (cp < 0x800) {
    out += (char) (0xC0 | (cp >> 6));
    out += (char) (0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += (char) (0xE0 | (cp >> 12));
    out += (char) (0x80 | ((cp >> 6) & 0x3F));
    out += (char) (0x80 | (cp & 0x3F));
  } else {
    out += (char) (0xF0 | (cp >> 18));
    out += (char) (0x80 | ((cp >> 12) & 0x3F));
    out += (char) (0x80 | ((cp >> 6) & 0x3F));
    out += (char) (0x80 | (cp & 0x3F));
  }
}

static uint32_t cp_upper(uint32_t c) {
  if (c >= 'a' && c <= 'z')
    return c - 32;
  if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
    return c - 32;
  // Latin Extended-A: pairs (upper even) 0100-0137 and 014A-0177,
  // (upper odd) 0139-0148 and 0179-017E.
  if ((c >= 0x100 && c <= 0x137) || (c >= 0x14A && c <= 0x177))
    return (c & 1) ? c - 1 : c;
  if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
    return (c & 1) ? c : c - 1;
  return c;
}

static uint32_t cp_lower(uint32_t c) {
  if (c >= 'A' && c <= 'Z')
    return c + 32;
  if (c >= 0xC0 && c <= 0xDE && c != 0xD7)
    return c + 32;
  if ((c >= 0x100 && c <= 0x137) || (c >= 0x14A && c <= 0x177))
    return (c & 1) ? c : c + 1;
  if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
    return (c & 1) ? c + 1 : c;
  return c;
}

std::string upper(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size();)
    encode(cp_upper(decode(s, i)), out);
  return out;
}

static std::string map_first(const std::string &s, uint32_t (*fn)(uint32_t)) {
  if (s.empty())
    return s;
  size_t i = 0;
  uint32_t c = decode(s, i);
  std::string out;
  encode(fn(c), out);
  out += s.substr(i);
  return out;
}

std::string cap_first(const std::string &s) { return map_first(s, cp_upper); }
std::string lower_first(const std::string &s) { return map_first(s, cp_lower); }

static std::string lower(const std::string &s) {
  std::string out;
  for (size_t i = 0; i < s.size();)
    encode(cp_lower(decode(s, i)), out);
  return out;
}

static bool starts(const std::string &s, const char *p) { return s.compare(0, strlen(p), p) == 0; }

std::string weekday_short(const std::string &weekday) {
  std::string w = lower(weekday);
  if (starts(w, "pon"))
    return "pon";
  if (starts(w, "wt"))
    return "wt";
  if (starts(w, "śr"))
    return "śr";
  if (starts(w, "czw"))
    return "czw";
  if (starts(w, "pi") || starts(w, "pt"))
    return "pt";
  if (starts(w, "sob"))
    return "sob";
  if (starts(w, "nie"))
    return "niedz";
  return w.substr(0, std::min<size_t>(3, w.size()));
}

std::string ddmm(const std::string &date) {
  static const char *const MONTHS[12] = {"sty", "lut", "mar", "kwi", "maj", "cze",
                                         "lip", "sie", "wrz", "paź", "lis", "gru"};
  char *end = nullptr;
  long day = strtol(date.c_str(), &end, 10);
  if (end == date.c_str() || day < 1 || day > 31)
    return date;
  std::string rest = lower(std::string(end));
  size_t p = rest.find_first_not_of(" .");
  if (p == std::string::npos)
    return date;
  rest = rest.substr(p);
  for (int k = 0; k < 12; k++) {
    if (starts(rest, MONTHS[k])) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%02ld.%02d", day, k + 1);
      return buf;
    }
  }
  return date;
}

std::string first_clause(const std::string &s) {
  size_t p = s.find(',');
  return p == std::string::npos ? s : s.substr(0, p);
}

std::string fit(Font *f, const std::string &s, int max_w, float ls) {
  if (f == nullptr || measure(f, s, ls) <= max_w)
    return s;
  // Drop code points from the end until "text…" fits.
  std::vector<size_t> starts_at;
  for (size_t i = 0; i < s.size();) {
    starts_at.push_back(i);
    decode(s, i);
  }
  for (size_t n = starts_at.size(); n > 0; n--) {
    std::string t = s.substr(0, starts_at[n - 1]);
    while (!t.empty() && t.back() == ' ')
      t.pop_back();
    t += "…";
    if (measure(f, t, ls) <= max_w)
      return t;
  }
  return "…";
}

std::vector<std::string> wrap_lines(Font *f, const std::string &s, int max_w, int max_lines) {
  std::vector<std::string> lines = wrap(f, s, max_w);
  if (max_lines > 0 && (int) lines.size() > max_lines) {
    lines.resize(max_lines);
    lines.back() = fit(f, lines.back() + "…", max_w);
  }
  for (auto &l : lines)
    l = fit(f, l, max_w);  // a single word wider than the box
  return lines;
}

std::vector<std::string> split_words(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  for (char ch : s) {
    if (ch == ' ' || ch == '\n' || ch == '\t') {
      if (!cur.empty())
        out.push_back(cur);
      cur.clear();
    } else {
      cur += ch;
    }
  }
  if (!cur.empty())
    out.push_back(cur);
  return out;
}

// ---------------------------------------------------------------------------
// Numbers

std::string deg(float v) {
  if (std::isnan(v))
    return "–";
  char buf[32];
  snprintf(buf, sizeof(buf), "%d°", (int) std::lround(v));
  return buf;
}

std::string signed_deg(float v) {
  if (std::isnan(v))
    return "–";
  int i = (int) std::lround(v);
  char buf[32];
  if (i > 0)
    snprintf(buf, sizeof(buf), "+%d°", i);
  else
    snprintf(buf, sizeof(buf), "%d°", i);
  return buf;
}

std::string signed_int(int v) {
  char buf[32];
  if (v >= 0)
    snprintf(buf, sizeof(buf), "+%d", v);
  else
    snprintf(buf, sizeof(buf), "%d", v);
  return buf;
}

std::string fmt1(float v) {
  if (std::isnan(v))
    return "–";
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f", v);
  return buf;
}

std::string dur_label(int s) {
  if (s < 0)
    s = 0;
  char buf[32];
  if (s % 60 == 0)
    snprintf(buf, sizeof(buf), "%d min", s / 60);
  else if (s < 60)
    snprintf(buf, sizeof(buf), "%d s", s);
  else
    snprintf(buf, sizeof(buf), "%d:%02d", s / 60, s % 60);
  return buf;
}

bool parse_hhmm(const std::string &s, int &h, int &m) {
  int a, b;
  if (sscanf(s.c_str(), "%d:%d", &a, &b) != 2 || a < 0 || a > 23 || b < 0 || b > 59)
    return false;
  h = a;
  m = b;
  return true;
}

// ---------------------------------------------------------------------------
// Model

float now_hours(const Model &m) {
  float h = m.hour + m.minute / 60.0f;
  return std::min(22.0f, std::max(6.0f, h));
}

float timer_frac(const Model &m) {
  if (m.timer_total <= 0)
    return 0;
  float f = (float) m.timer_remaining / (float) m.timer_total;
  return std::min(1.0f, std::max(0.0f, f));
}

// Hourly temperature; a missing (NAN) hour takes the nearest known one, so a
// short array from HA does not drop the curve to 0 degrees.
static float fc_at(const Model &m, int i) {
  const int n = (int) m.fc_temp.size();
  i = std::max(0, std::min(n - 1, i));
  for (int k = 0; k < n; k++) {
    if (i - k >= 0 && !std::isnan(m.fc_temp[i - k]))
      return m.fc_temp[i - k];
    if (i + k < n && !std::isnan(m.fc_temp[i + k]))
      return m.fc_temp[i + k];
  }
  return 0.0f;
}

float fc_temp_at(const Model &m, float h) {
  float x = std::min(24.0f, std::max(6.0f, h)) - 6.0f;
  int i = (int) std::floor(x);
  if (i >= (int) m.fc_temp.size() - 1)
    return fc_at(m, (int) m.fc_temp.size() - 1);
  float u = (1.0f - std::cos(3.14159265f * (x - i))) / 2.0f;
  float a = fc_at(m, i), b = fc_at(m, i + 1);
  return a + (b - a) * u;
}

void fc_range(const Model &m, float &lo, float &hi) {
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
    hi = 0;
  }
}

int fc_peak_index(const Model &m) {
  int best = 0;
  for (int i = 1; i < (int) m.fc_temp.size(); i++)
    if (fc_at(m, i) > fc_at(m, best))
      best = i;
  return best;
}

float fc_rain_max(const Model &m) {
  float mx = 0;
  for (float v : m.fc_rain)
    if (!std::isnan(v))
      mx = std::max(mx, v);
  return mx;
}

float fc_rain_sum(const Model &m) {
  float s = 0;
  for (size_t i = 0; i + 1 < m.fc_rain.size(); i++)  // 19 values = hours 6..24; 24 is the next day
    if (!std::isnan(m.fc_rain[i]))
      s += m.fc_rain[i];
  return s;
}

bool fc_has_rain(const Model &m) { return fc_rain_max(m) >= 0.05f; }

std::string brief_text(const Model &m) { return m.now_body.empty() ? m.now_title : m.now_body; }

std::string now_kind(const std::string &icon) {
  if (icon == "window")
    return "Okno";
  if (icon == "wind")
    return "Ostrzeżenie";
  if (icon == "plant")
    return "Roślina";
  if (icon == "vacuum")
    return "Odkurzacz";
  if (icon == "school")
    return "Szkoła";
  if (icon == "flood")
    return "Zalanie";
  if (icon == "smoke")
    return "Dym";
  if (icon == "brief")
    return "Dzień";
  return "Uwaga";
}

std::string countdown(const Model &m) {
  int h, mi;
  if (!parse_hhmm(m.next_time, h, mi))
    return "";
  int diff = (h * 60 + mi) - (m.hour * 60 + m.minute);
  if (diff < 0)
    diff += 24 * 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "T−%02d:%02d", diff / 60, diff % 60);
  return buf;
}

// ---------------------------------------------------------------------------
// Answer typing

AnswerLayout layout_answer(Font *f, const Model &m, int max_w, int max_lines) {
  AnswerLayout L;
  L.max_lines = max_lines;
  std::vector<std::string> words = split_words(m.answer);
  const int n = (int) words.size();
  int shown = m.answer_words_shown < 0 ? n : std::min(n, m.answer_words_shown);
  L.typing = shown < n;

  // Greedy wrap of the complete answer, as word ranges.
  std::vector<std::pair<int, int>> ranges;  // [first, last)
  {
    int start = 0;
    std::string line;
    for (int i = 0; i < n; i++) {
      std::string trial = line.empty() ? words[i] : line + " " + words[i];
      if (!line.empty() && measure(f, trial) > max_w) {
        ranges.emplace_back(start, i);
        start = i;
        line = words[i];
      } else {
        line = trial;
      }
    }
    if (n > 0)
      ranges.emplace_back(start, n);
  }
  auto join = [&](int a, int b) {
    std::string s;
    for (int i = a; i < b; i++) {
      if (i > a)
        s += ' ';
      s += words[i];
    }
    return s;
  };

  // Line holding the last shown word.
  int cur = 0;
  for (int i = 0; i < (int) ranges.size(); i++)
    if (shown > ranges[i].first)
      cur = i;

  int first = 0;
  int total = (int) ranges.size();
  if (L.typing) {
    if (cur >= max_lines) {
      first = cur - max_lines + 1;
      L.scrolled = true;
    }
    for (int i = first; i <= cur && i < total; i++) {
      int b = std::min(ranges[i].second, shown);
      L.lines.push_back(join(ranges[i].first, std::max(ranges[i].first, b)));
    }
    if (L.lines.empty())
      L.lines.emplace_back();
    L.cursor_line = (int) L.lines.size() - 1;
  } else {
    for (int i = 0; i < total && i < max_lines; i++)
      L.lines.push_back(join(ranges[i].first, ranges[i].second));
    if (total > max_lines && !L.lines.empty())
      L.lines.back() = fit(f, L.lines.back() + "…", max_w);
    L.cursor_line = L.lines.empty() ? 0 : (int) L.lines.size() - 1;
  }
  L.cursor_w = L.lines.empty() ? 0 : measure(f, L.lines[L.cursor_line]);
  return L;
}

std::vector<Rect> typing_rects(const AnswerLayout &L, int x, int w, int y0, int pitch, int up, int h) {
  if (!L.typing)
    return {};
  if (L.scrolled)
    return {Rect{x, y0 - up, w, (L.max_lines - 1) * pitch + h}};
  int top = std::max(0, L.cursor_line - 1);
  return {Rect{x, y0 + top * pitch - up, w, (L.cursor_line - top) * pitch + h}};
}

// ---------------------------------------------------------------------------
// Pictograms

static void row_run(Display &d, int x0, int x1, int y, Color c) {
  if (y < 0 || y >= H)
    return;
  x0 = std::max(0, x0);
  x1 = std::min(W, x1);
  if (x1 > x0)
    d.horizontal_line(x0, y, x1 - x0, c);
}

static bool in_cloud(float px, float py, float cx, float cy, float s, float grow) {
  static const float C[3][3] = {{-0.5f, 0.1f, 0.32f}, {-0.05f, -0.15f, 0.45f}, {0.45f, 0.05f, 0.35f}};
  for (auto &k : C) {
    float dx = px - (cx + k[0] * s), dy = py - (cy + k[1] * s), r = k[2] * s + grow;
    if (dx * dx + dy * dy <= r * r)
      return true;
  }
  return px >= cx - 0.5f * s && px < cx + 0.45f * s && py >= cy + 0.05f * s && py < cy + 0.42f * s + grow;
}

void cloud(Display &d, float cx, float cy, float s, uint8_t top, uint8_t bottom, bool outline, Color outline_c) {
  const float g = outline ? 2.0f : 0.0f;
  int x0 = (int) std::floor(cx - 0.82f * s - g), x1 = (int) std::ceil(cx + 0.8f * s + g) + 1;
  int y0 = (int) std::floor(cy - 0.6f * s - g), y1 = (int) std::ceil(cy + 0.42f * s + g) + 1;
  const float gy0 = cy - 0.6f * s, gy1 = cy + 0.45f * s;
  for (int y = std::max(0, y0); y < std::min(H, y1); y++) {
    const float py = y + 0.5f;
    float u = std::min(1.0f, std::max(0.0f, (py - gy0) / (gy1 - gy0)));
    Color fill = tone((uint8_t) std::lround(top + (bottom - top) * u));
    int start = -1, kind = 0;  // 0 none, 1 fill, 2 outline
    for (int x = std::max(0, x0); x <= std::min(W, x1); x++) {
      int k = 0;
      if (x < std::min(W, x1)) {
        const float px = x + 0.5f;
        if (in_cloud(px, py, cx, cy, s, 0))
          k = 1;
        else if (outline && in_cloud(px, py, cx, cy, s, g))
          k = 2;
      }
      if (k != kind) {
        if (kind != 0)
          row_run(d, start, x, y, kind == 1 ? fill : outline_c);
        start = x;
        kind = k;
      }
    }
  }
}

void sun(Display &d, float cx, float cy, float r, uint8_t inner, uint8_t outer, bool rays, Color line_c) {
  const float fx = cx - r * 0.3f, fy = cy - r * 0.3f, span = r * 1.3f;
  for (int y = (int) std::floor(cy - r); y <= (int) std::ceil(cy + r); y++) {
    const float py = y + 0.5f;
    for (int x = (int) std::floor(cx - r); x <= (int) std::ceil(cx + r); x++) {
      const float px = x + 0.5f;
      float dx = px - cx, dy = py - cy;
      if (dx * dx + dy * dy > r * r || x < 0 || x >= W || y < 0 || y >= H)
        continue;
      float t = std::min(1.0f, std::hypot(px - fx, py - fy) / span);
      d.draw_pixel_at(x, y, tone((uint8_t) std::lround(inner + (outer - inner) * t)));
    }
  }
  if (rays) {
    for (int i = 0; i < 8; i++) {
      float a = i * 3.14159265f / 4;
      thick_line(d, cx + std::cos(a) * (r + 4), cy + std::sin(a) * (r + 4), cx + std::cos(a) * (r + 9),
                 cy + std::sin(a) * (r + 9), 2, line_c);
    }
    ring(d, cx, cy, r, 1.6f, line_c);
  }
}

void moon(Display &d, float cx, float cy, float r, uint8_t g, bool rim) {
  const float ox = cx + r * 0.45f, oy = cy - r * 0.3f, orr = r * 0.85f;
  for (int y = (int) std::floor(cy - r - 1); y <= (int) std::ceil(cy + r + 1); y++) {
    for (int x = (int) std::floor(cx - r - 1); x <= (int) std::ceil(cx + r + 1); x++) {
      if (x < 0 || x >= W || y < 0 || y >= H)
        continue;
      float px = x + 0.5f, py = y + 0.5f;
      float d1 = std::hypot(px - cx, py - cy), d2 = std::hypot(px - ox, py - oy);
      bool in = d1 <= r && d2 > orr;
      bool edge = rim && !in && d1 <= r + 1.6f && d2 > orr - 1.6f;
      if (in)
        d.draw_pixel_at(x, y, tone(g));
      else if (edge)
        d.draw_pixel_at(x, y, ink(0));
    }
  }
}

void bolt(Display &d, float x, float y, float s, Color c) {
  fill_poly(d,
            {{x + 0.55f * s, y},
             {x + 0.1f * s, y + 0.55f * s},
             {x + 0.42f * s, y + 0.55f * s},
             {x + 0.25f * s, y + s},
             {x + 0.8f * s, y + 0.4f * s},
             {x + 0.48f * s, y + 0.4f * s},
             {x + 0.7f * s, y}},
            c);
}

void weather_icon(Display &d, const std::string &cond, float cx, float cy, float s, const IconStyle &st) {
  auto the_cloud = [&](float x, float y, float sc) {
    cloud(d, x, y, sc, st.cloud_top, st.cloud_bottom, st.outline);
  };
  auto the_sun = [&](float x, float y, float r) {
    sun(d, x, y, r, st.sun_inner, st.sun_outer, st.sun_rays, ink(0));
  };
  auto strokes = [&](int n) {
    for (int i = 0; i < n; i++) {
      float x = cx + (-0.435f + i * (0.913f / (n - 1))) * s;
      thick_line(d, x, cy + 0.565f * s, x - 0.11f * s, cy + 0.87f * s, st.line_w, st.line, st.round);
    }
  };
  if (cond == "sunny") {
    the_sun(cx + 0.1f * s, cy - 0.05f * s, 0.42f * s);
  } else if (cond == "clear-night") {
    moon(d, cx + 0.1f * s, cy - 0.05f * s, 0.45f * s, st.cloud_top, st.outline);
  } else if (cond == "partlycloudy") {
    the_sun(cx + 0.944f * s, cy - 0.667f * s, 0.36f * s);
    the_cloud(cx, cy, s);
  } else if (cond == "cloudy") {
    the_cloud(cx + 0.55f * s, cy - 0.42f * s, 0.62f * s);
    the_cloud(cx, cy, s);
  } else if (cond == "fog") {
    the_cloud(cx, cy, s * 0.9f);
    thick_line(d, cx - 0.6f * s, cy + 0.62f * s, cx + 0.3f * s, cy + 0.62f * s, st.line_w, st.line, true);
    thick_line(d, cx - 0.35f * s, cy + 0.82f * s, cx + 0.6f * s, cy + 0.82f * s, st.line_w, st.line, true);
  } else if (cond == "rainy" || cond == "pouring") {
    the_cloud(cx, cy, s);
    strokes(cond == "pouring" ? 6 : 4);
  } else if (cond == "snowy" || cond == "snowy-rainy" || cond == "hail") {
    the_cloud(cx, cy, s);
    for (int i = 0; i < 4; i++) {
      float x = cx + (-0.435f + i * 0.304f) * s;
      fill_circle(d, x, cy + 0.66f * s + (i & 1) * 0.16f * s, std::max(1.5f, 0.055f * s), st.line);
    }
  } else if (cond == "lightning" || cond == "lightning-rainy") {
    the_cloud(cx, cy, s);
    bolt(d, cx - 0.2f * s, cy + 0.4f * s, 0.55f * s, st.line);
  } else if (cond == "windy" || cond == "windy-variant") {
    if (cond == "windy-variant")
      the_cloud(cx + 0.3f * s, cy - 0.35f * s, 0.6f * s);
    thick_line(d, cx - 0.7f * s, cy - 0.1f * s, cx + 0.45f * s, cy - 0.1f * s, st.line_w, st.line, true);
    thick_line(d, cx - 0.5f * s, cy + 0.15f * s, cx + 0.75f * s, cy + 0.15f * s, st.line_w, st.line, true);
    thick_line(d, cx - 0.7f * s, cy + 0.4f * s, cx + 0.25f * s, cy + 0.4f * s, st.line_w, st.line, true);
  } else {
    the_cloud(cx, cy, s);
  }
}

void keycap(Display &d, Font *f, Sym s, const std::string &label, int x, int y, Align a, Color c, Color bg) {
  const int tw = 21 + measure(f, label);
  const int x0 = a == Align::LEFT ? x : a == Align::RIGHT ? x - tw : (int) std::lround(x - tw / 2.0f);
  stroke_round(d, x0, y - 12, 16, 16, 3, c, 1);
  sym(d, s, x0 + 4, y - 1, 7, c);
  text(d, f, x0 + 21, y, c, label, Align::LEFT, bg);
}

void hint(Display &d, Font *f, Sym s, const std::string &label, int x, int y, Align a, Color c, Color bg) {
  const int tw = 13 + measure(f, label);
  const int x0 = a == Align::LEFT ? x : a == Align::RIGHT ? x - tw : (int) std::lround(x - tw / 2.0f);
  sym(d, s, x0, y - 1, 8, c);
  text(d, f, x0 + 13, y, c, label, Align::LEFT, bg);
}

void check(Display &d, float x, float y, float s, float w, Color c) {
  polyline(d, {{x, y + 0.5f * s}, {x + 0.35f * s, y + 0.85f * s}, {x + s, y}}, w, c);
}

}  // namespace uu
}  // namespace note4_ui
}  // namespace esphome
