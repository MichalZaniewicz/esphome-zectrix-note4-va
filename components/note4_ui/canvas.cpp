// Drawing helpers declared in note4_ui.h.
//
// Geometry follows the HTML canvas mockups: a pixel is inside a shape when its
// centre (x + 0.5, y + 0.5) is. No anti-aliasing (the panel is 1-bit or 16 grey).
// Shapes are filled as horizontal runs, one Display::horizontal_line per run.
#include "note4_ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace esphome {
namespace note4_ui {

static const float TAU_F = 6.28318531f;

// ---------------------------------------------------------------------------
// Low level: spans

// Pixels [x0, x1) on row y.
static void run(Display &d, int x0, int x1, int y, Color c) {
  if (y < 0 || y >= H)
    return;
  if (x0 < 0)
    x0 = 0;
  if (x1 > W)
    x1 = W;
  if (x1 <= x0)
    return;
  d.horizontal_line(x0, y, x1 - x0, c);
}

// Pixels whose centres lie in [a, b) on row y.
static void span(Display &d, float a, float b, int y, Color c) {
  int x0 = (int) std::ceil(a - 0.5f);
  int x1 = (int) std::ceil(b - 0.5f);
  run(d, x0, x1, y, c);
}

// Row-wise rasteriser for shapes given as an inside(px, py) test on pixel
// centres. Bounding box in pixels, inclusive-exclusive.
template<typename F> static void raster(Display &d, int bx0, int by0, int bx1, int by1, Color c, F inside) {
  bx0 = std::max(bx0, 0);
  by0 = std::max(by0, 0);
  bx1 = std::min(bx1, W);
  by1 = std::min(by1, H);
  for (int y = by0; y < by1; y++) {
    const float py = y + 0.5f;
    int start = -1;
    for (int x = bx0; x < bx1; x++) {
      bool in = inside(x + 0.5f, py);
      if (in && start < 0) {
        start = x;
      } else if (!in && start >= 0) {
        run(d, start, x, y, c);
        start = -1;
      }
    }
    if (start >= 0)
      run(d, start, bx1, y, c);
  }
}

// ---------------------------------------------------------------------------
// UTF-8

// Decodes one code point at s[i]; advances i. Invalid bytes decode as '?'.
static uint32_t next_cp(const std::string &s, size_t &i) {
  const uint8_t c0 = (uint8_t) s[i];
  int len = c0 < 0x80 ? 1 : (c0 >> 5) == 0x6 ? 2 : (c0 >> 4) == 0xE ? 3 : (c0 >> 3) == 0x1E ? 4 : 0;
  if (len == 0 || i + len > s.size()) {
    i++;
    return '?';
  }
  uint32_t cp = len == 1 ? c0 : len == 2 ? (c0 & 0x1F) : len == 3 ? (c0 & 0x0F) : (c0 & 0x07);
  for (int k = 1; k < len; k++) {
    uint8_t cc = (uint8_t) s[i + k];
    if ((cc & 0xC0) != 0x80) {
      i++;
      return '?';
    }
    cp = (cp << 6) | (cc & 0x3F);
  }
  i += len;
  return cp;
}

static int advance_of(Font *f, uint32_t cp, const char *utf8) {
  const font::Glyph *g = f->find_glyph(cp);
  if (g != nullptr)
    return g->advance;
  int w, xo, bl, h;
  f->measure(utf8, &w, &xo, &bl, &h);
  return w + xo;
}

static display::TextAlign baseline_align(Align a) {
  switch (a) {
    case Align::CENTER:
      return display::TextAlign::BASELINE_CENTER;
    case Align::RIGHT:
      return display::TextAlign::BASELINE_RIGHT;
    default:
      return display::TextAlign::BASELINE_LEFT;
  }
}

// ---------------------------------------------------------------------------
// Text

int measure(Font *f, const std::string &s, float ls) {
  if (f == nullptr || s.empty())
    return 0;
  int w, xo, bl, h;
  f->measure(s.c_str(), &w, &xo, &bl, &h);
  float total = (float) (w + xo);
  if (ls != 0) {
    int n = 0;
    for (size_t i = 0; i < s.size();) {
      next_cp(s, i);
      n++;
    }
    total += ls * n;  // canvas letterSpacing: after every glyph
  }
  return (int) std::lround(total);
}

void text(Display &d, Font *f, int x, int y, Color c, const std::string &s, Align a, Color bg, float ls) {
  if (f == nullptr || s.empty())
    return;
  if (ls == 0) {
    d.print(x, y, f, c, baseline_align(a), s.c_str(), bg);
    return;
  }
  const int total = measure(f, s, ls);
  float pos = (float) x;
  if (a == Align::CENTER)
    pos = x - total / 2.0f;
  else if (a == Align::RIGHT)
    pos = (float) (x - total);
  char buf[32];
  for (size_t i = 0; i < s.size();) {
    size_t start = i;
    uint32_t cp = next_cp(s, i);
    size_t n = std::min<size_t>(i - start, sizeof(buf) - 1);
    std::copy(s.begin() + start, s.begin() + start + n, buf);
    buf[n] = 0;
    if (cp != ' ')
      d.print((int) std::lround(pos), y, f, c, display::TextAlign::BASELINE_LEFT, buf, bg);
    pos += advance_of(f, cp, buf) + ls;
  }
}

std::vector<std::string> wrap(Font *f, const std::string &s, int max_w) {
  std::vector<std::string> out;
  if (f == nullptr)
    return out;
  // Paragraphs on '\n', words on spaces.
  size_t p = 0;
  while (p <= s.size()) {
    size_t nl = s.find('\n', p);
    std::string para = s.substr(p, nl == std::string::npos ? std::string::npos : nl - p);
    std::string line;
    size_t i = 0;
    while (i < para.size()) {
      while (i < para.size() && para[i] == ' ')
        i++;
      if (i >= para.size())
        break;
      size_t j = para.find(' ', i);
      if (j == std::string::npos)
        j = para.size();
      std::string word = para.substr(i, j - i);
      i = j;
      std::string trial = line.empty() ? word : line + " " + word;
      if (!line.empty() && measure(f, trial) > max_w) {
        out.push_back(line);
        line = word;
      } else {
        line = trial;
      }
    }
    if (!line.empty())
      out.push_back(line);
    if (nl == std::string::npos)
      break;
    p = nl + 1;
  }
  return out;
}

void fixed_digits(Display &d, Font *f, int cx, int y, Color c, const std::string &s, Color bg) {
  if (f == nullptr)
    return;
  int cw = 0;
  for (char ch = '0'; ch <= '9'; ch++) {
    char b[2] = {ch, 0};
    cw = std::max(cw, advance_of(f, (uint32_t) ch, b));
  }
  const int colw = advance_of(f, ':', ":") + 4;
  // Cell widths.
  std::vector<std::pair<std::string, int>> cells;
  float total = 0;
  for (size_t i = 0; i < s.size();) {
    size_t start = i;
    uint32_t cp = next_cp(s, i);
    std::string ch = s.substr(start, i - start);
    int w = (cp >= '0' && cp <= '9') ? cw : cp == ':' ? colw : advance_of(f, cp, ch.c_str());
    cells.emplace_back(ch, w);
    total += w;
  }
  float x = cx - total / 2.0f;
  for (auto &cell : cells) {
    if (cell.first != " ")
      d.print((int) std::lround(x + cell.second / 2.0f), y, f, c, display::TextAlign::BASELINE_CENTER,
              cell.first.c_str(), bg);
    x += cell.second;
  }
}

// ---------------------------------------------------------------------------
// Rectangles and lines

void fill_rect(Display &d, int x, int y, int w, int h, Color c) {
  if (w <= 0 || h <= 0)
    return;
  int x0 = std::max(x, 0), y0 = std::max(y, 0);
  int x1 = std::min(x + w, W), y1 = std::min(y + h, H);
  if (x1 <= x0 || y1 <= y0)
    return;
  d.filled_rectangle(x0, y0, x1 - x0, y1 - y0, c);
}

void stroke_rect(Display &d, int x, int y, int w, int h, Color c, int t) {
  if (w <= 0 || h <= 0 || t <= 0)
    return;
  if (2 * t >= w || 2 * t >= h) {
    fill_rect(d, x, y, w, h, c);
    return;
  }
  fill_rect(d, x, y, w, t, c);
  fill_rect(d, x, y + h - t, w, t, c);
  fill_rect(d, x, y + t, t, h - 2 * t, c);
  fill_rect(d, x + w - t, y + t, t, h - 2 * t, c);
}

// Horizontal extent [a, b) of a rounded rect at row centre py. False if outside.
static bool round_span(float x, float y, float w, float h, float r, float py, float &a, float &b) {
  if (py < y || py >= y + h)
    return false;
  float dx = 0;
  if (r > 0) {
    float dy = 0;
    if (py < y + r)
      dy = y + r - py;
    else if (py > y + h - r)
      dy = py - (y + h - r);
    if (dy > 0) {
      float q = r * r - dy * dy;
      dx = r - (q > 0 ? std::sqrt(q) : 0);
    }
  }
  a = x + dx;
  b = x + w - dx;
  return b > a;
}

void fill_round(Display &d, int x, int y, int w, int h, int r, Color c) {
  if (w <= 0 || h <= 0)
    return;
  float rr = std::min<float>((float) r, std::min(w, h) / 2.0f);
  for (int row = y; row < y + h; row++) {
    float a, b;
    if (round_span(x, y, w, h, rr, row + 0.5f, a, b))
      span(d, a, b, row, c);
  }
}

void stroke_round(Display &d, int x, int y, int w, int h, int r, Color c, int t) {
  if (w <= 0 || h <= 0 || t <= 0)
    return;
  float ro = std::min<float>((float) r, std::min(w, h) / 2.0f);
  float ri = std::max(0.0f, ro - t);
  for (int row = y; row < y + h; row++) {
    float py = row + 0.5f, a, b, ia, ib;
    if (!round_span(x, y, w, h, ro, py, a, b))
      continue;
    if (round_span(x + t, y + t, w - 2 * t, h - 2 * t, ri, py, ia, ib)) {
      span(d, a, ia, row, c);
      span(d, ib, b, row, c);
    } else {
      span(d, a, b, row, c);
    }
  }
}

void hline(Display &d, int x1, int x2, int y, Color c, int t) {
  if (x2 < x1)
    std::swap(x1, x2);
  fill_rect(d, x1, y, x2 - x1, t, c);
}

void vline(Display &d, int x, int y1, int y2, Color c, int t) {
  if (y2 < y1)
    std::swap(y1, y2);
  fill_rect(d, x, y1, t, y2 - y1, c);
}

// Patterns are anchored to the screen origin, like a canvas pattern.
void dotted_hline(Display &d, int x1, int x2, int y, Color c, int on, int off) {
  if (x2 < x1)
    std::swap(x1, x2);
  const int period = std::max(1, on + off);
  int x = x1;
  while (x < x2) {
    int phase = ((x % period) + period) % period;
    if (phase < on) {
      int end = std::min(x2, x + (on - phase));
      run(d, x, end, y, c);
      x = end;
    } else {
      x += period - phase;
    }
  }
}

void dotted_vline(Display &d, int x, int y1, int y2, Color c, int on, int off) {
  if (y2 < y1)
    std::swap(y1, y2);
  const int period = std::max(1, on + off);
  for (int y = y1; y < y2; y++) {
    int phase = ((y % period) + period) % period;
    if (phase < on && x >= 0 && x < W && y >= 0 && y < H)
      d.draw_pixel_at(x, y, c);
  }
}

// ---------------------------------------------------------------------------
// Polygons

void fill_poly(Display &d, const std::vector<std::pair<float, float>> &pts, Color c) {
  const size_t n = pts.size();
  if (n < 3)
    return;
  float ymin = pts[0].second, ymax = ymin;
  for (auto &p : pts) {
    ymin = std::min(ymin, p.second);
    ymax = std::max(ymax, p.second);
  }
  int y0 = std::max(0, (int) std::floor(ymin));
  int y1 = std::min(H - 1, (int) std::ceil(ymax));
  std::vector<float> xs;
  xs.reserve(n);
  for (int y = y0; y <= y1; y++) {
    const float py = y + 0.5f;
    xs.clear();
    for (size_t i = 0; i < n; i++) {
      const auto &p = pts[i];
      const auto &q = pts[(i + 1) % n];
      float ya = p.second, yb = q.second;
      if (ya == yb)
        continue;
      // Half-open in y so shared vertices count once.
      if ((py >= ya && py < yb) || (py >= yb && py < ya)) {
        float t = (py - ya) / (yb - ya);
        xs.push_back(p.first + t * (q.first - p.first));
      }
    }
    if (xs.size() < 2)
      continue;
    std::sort(xs.begin(), xs.end());
    for (size_t i = 0; i + 1 < xs.size(); i += 2)
      span(d, xs[i], xs[i + 1], y, c);
  }
}

void fill_circle(Display &d, float cx, float cy, float r, Color c) {
  if (r <= 0)
    return;
  int y0 = (int) std::floor(cy - r), y1 = (int) std::ceil(cy + r);
  for (int y = y0; y <= y1; y++) {
    float dy = y + 0.5f - cy;
    float q = r * r - dy * dy;
    if (q < 0)
      continue;
    float dx = std::sqrt(q);
    span(d, cx - dx, cx + dx, y, c);
  }
}

static void thin_line(Display &d, float x1, float y1, float x2, float y2, Color c) {
  float dx = x2 - x1, dy = y2 - y1;
  int steps = (int) std::ceil(std::max(std::fabs(dx), std::fabs(dy)));
  if (steps == 0) {
    int px = (int) std::floor(x1), py = (int) std::floor(y1);
    if (px >= 0 && px < W && py >= 0 && py < H)
      d.draw_pixel_at(px, py, c);
    return;
  }
  for (int i = 0; i <= steps; i++) {
    float t = (float) i / steps;
    int px = (int) std::floor(x1 + dx * t), py = (int) std::floor(y1 + dy * t);
    if (px >= 0 && px < W && py >= 0 && py < H)
      d.draw_pixel_at(px, py, c);
  }
}

void thick_line(Display &d, float x1, float y1, float x2, float y2, float w, Color c, bool round_caps) {
  if (w < 1.4f) {
    thin_line(d, x1, y1, x2, y2, c);
    return;
  }
  float dx = x2 - x1, dy = y2 - y1;
  float len = std::sqrt(dx * dx + dy * dy);
  const float hw = w / 2.0f;
  if (len > 0.001f) {
    float nx = -dy / len * hw, ny = dx / len * hw;
    fill_poly(d, {{x1 + nx, y1 + ny}, {x2 + nx, y2 + ny}, {x2 - nx, y2 - ny}, {x1 - nx, y1 - ny}}, c);
  }
  if (round_caps) {
    fill_circle(d, x1, y1, hw, c);
    fill_circle(d, x2, y2, hw, c);
  }
}

void polyline(Display &d, const std::vector<std::pair<float, float>> &pts, float w, Color c) {
  if (pts.size() < 2)
    return;
  for (size_t i = 0; i + 1 < pts.size(); i++)
    thick_line(d, pts[i].first, pts[i].second, pts[i + 1].first, pts[i + 1].second, w, c, false);
  // Round joins.
  if (w >= 1.4f)
    for (size_t i = 1; i + 1 < pts.size(); i++)
      fill_circle(d, pts[i].first, pts[i].second, w / 2.0f, c);
}

// ---------------------------------------------------------------------------
// Circles, arcs, wedges, ellipses

void ring(Display &d, float cx, float cy, float r, float t, Color c) {
  const float ro = r + t / 2.0f, ri = std::max(0.0f, r - t / 2.0f);
  int y0 = (int) std::floor(cy - ro), y1 = (int) std::ceil(cy + ro);
  for (int y = y0; y <= y1; y++) {
    float dy = y + 0.5f - cy;
    float qo = ro * ro - dy * dy;
    if (qo < 0)
      continue;
    float xo = std::sqrt(qo);
    float qi = ri * ri - dy * dy;
    if (qi > 0) {
      float xi = std::sqrt(qi);
      span(d, cx - xo, cx - xi, y, c);
      span(d, cx + xi, cx + xo, y, c);
    } else {
      span(d, cx - xo, cx + xo, y, c);
    }
  }
}

// Angle of (dx, dy) relative to a0, in [0, 2pi).
static inline float rel_angle(float dx, float dy, float a0) {
  float a = std::atan2(dy, dx) - a0;
  a = std::fmod(a, TAU_F);
  if (a < 0)
    a += TAU_F;
  return a;
}

void arc(Display &d, float cx, float cy, float r, float t, float a0, float a1, Color c, bool round_caps) {
  float sweep = a1 - a0;
  if (sweep <= 0)
    return;
  if (sweep >= TAU_F - 1e-4f) {
    ring(d, cx, cy, r, t, c);
    return;
  }
  const float ro = r + t / 2.0f, ri = std::max(0.0f, r - t / 2.0f);
  const float ro2 = ro * ro, ri2 = ri * ri;
  raster(d, (int) std::floor(cx - ro), (int) std::floor(cy - ro), (int) std::ceil(cx + ro) + 1,
         (int) std::ceil(cy + ro) + 1, c, [&](float px, float py) {
           float dx = px - cx, dy = py - cy, q = dx * dx + dy * dy;
           if (q > ro2 || q < ri2)
             return false;
           return rel_angle(dx, dy, a0) <= sweep;
         });
  if (round_caps && t > 0) {
    fill_circle(d, cx + std::cos(a0) * r, cy + std::sin(a0) * r, t / 2.0f, c);
    fill_circle(d, cx + std::cos(a1) * r, cy + std::sin(a1) * r, t / 2.0f, c);
  }
}

void wedge(Display &d, float cx, float cy, float r, float a0, float a1, Color c) {
  float sweep = a1 - a0;
  if (sweep <= 0)
    return;
  if (sweep >= TAU_F - 1e-4f) {
    fill_circle(d, cx, cy, r, c);
    return;
  }
  const float r2 = r * r;
  raster(d, (int) std::floor(cx - r), (int) std::floor(cy - r), (int) std::ceil(cx + r) + 1, (int) std::ceil(cy + r) + 1,
         c, [&](float px, float py) {
           float dx = px - cx, dy = py - cy;
           if (dx * dx + dy * dy > r2)
             return false;
           return rel_angle(dx, dy, a0) <= sweep;
         });
}

void fill_ellipse(Display &d, float cx, float cy, float rx, float ry, float rot, Color c) {
  if (rx <= 0 || ry <= 0)
    return;
  const float cr = std::cos(rot), sr = std::sin(rot);
  const float m = std::max(rx, ry);
  raster(d, (int) std::floor(cx - m), (int) std::floor(cy - m), (int) std::ceil(cx + m) + 1, (int) std::ceil(cy + m) + 1,
         c, [&](float px, float py) {
           float dx = px - cx, dy = py - cy;
           float u = dx * cr + dy * sr, v = -dx * sr + dy * cr;
           return (u * u) / (rx * rx) + (v * v) / (ry * ry) <= 1.0f;
         });
}

// ---------------------------------------------------------------------------
// Bitmaps and symbols

void bitmap(Display &d, const std::vector<const char *> &rows, int x, int y, int s, Color c) {
  for (size_t j = 0; j < rows.size(); j++) {
    const char *row = rows[j];
    int i = 0;
    while (row[i] != 0) {
      if (row[i] == '1') {
        int start = i;
        while (row[i] == '1')
          i++;
        fill_rect(d, x + start * s, y + (int) j * s, (i - start) * s, s, c);
      } else {
        i++;
      }
    }
  }
}

void sym(Display &d, Sym s, int x, int y, int size, Color c) {
  const float fs = (float) size;
  switch (s) {
    case Sym::UP:
      fill_poly(d, {{(float) x, (float) y}, {x + fs, (float) y}, {x + fs / 2, y - fs * 0.85f}}, c);
      break;
    case Sym::DOWN:
      fill_poly(d, {{(float) x, y - fs * 0.85f}, {x + fs, y - fs * 0.85f}, {x + fs / 2, (float) y}}, c);
      break;
    case Sym::DOT:
      fill_circle(d, x + fs / 2, y - fs / 2 + 0.5f, fs / 2, c);
      break;
  }
}

// ---------------------------------------------------------------------------
std::string mmss(int seconds) {
  if (seconds < 0)
    seconds = 0;
  char buf[32];
  snprintf(buf, sizeof(buf), "%02d:%02d", seconds / 60, seconds % 60);
  return buf;
}

std::string hhmm(int h, int m) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
  return buf;
}

}  // namespace note4_ui
}  // namespace esphome
