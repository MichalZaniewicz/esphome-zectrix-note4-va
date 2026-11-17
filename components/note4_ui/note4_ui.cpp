#include "note4_ui.h"

#include <algorithm>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace note4_ui {

static const char *const TAG = "note4_ui";

// Never call the refresh handler more often than this.
static const uint32_t MIN_REQUEST_GAP_MS = 250;

[[maybe_unused]] static const char *refresh_name(Refresh r) {
  switch (r) {
    case Refresh::FULL_MONO:
      return "full mono";
    case Refresh::FULL_GRAY:
      return "full gray";
    case Refresh::PARTIAL:
      return "partial";
  }
  return "?";
}

// Word count of the answer, the same split the skins use (runs of spaces).
static int count_words(const std::string &s) {
  int n = 0;
  bool in_word = false;
  for (char ch : s) {
    bool sp = ch == ' ' || ch == '\n' || ch == '\t';
    if (!sp && !in_word)
      n++;
    in_word = !sp;
  }
  return n;
}

static bool answer_typing(const Model &m) {
  return m.answer_words_shown >= 0 && m.answer_words_shown < count_words(m.answer);
}

// ---------------------------------------------------------------------------
bool Skin::gray_allowed(Screen s) const {
  switch (s) {
    // Not ANSWER: a 16-grey refresh takes ~7.3 s on this panel (weather-glass
    // log), far too long to flash right after a spoken reply. Only the screens
    // you look at for a while get grey.
    case Screen::WEATHER:
    case Screen::DOORBELL:
      return true;
    default:
      return false;
  }
}

// ---------------------------------------------------------------------------
void Note4UI::setup() {
  if (this->skin_ == nullptr && !this->skins_.empty())
    this->skin_ = this->skins_.front();
  // First picture after boot.
  this->dirty_full_ = true;
  this->dirty_live_ = false;
}

void Note4UI::add_skin(Skin *s) {
  if (s == nullptr)
    return;
  for (auto *k : this->skins_) {
    if (std::string(k->id()) == s->id()) {
      ESP_LOGW(TAG, "Skin '%s' added twice, ignoring", s->id());
      return;
    }
  }
  this->skins_.push_back(s);
  ESP_LOGD(TAG, "Skin '%s' added", s->id());
  if (this->skin_ == nullptr) {
    this->skin_ = s;
    this->dirty_full_ = true;
    this->dirty_live_ = false;
  }
}

bool Note4UI::set_skin(const std::string &id) {
  for (auto *k : this->skins_) {
    if (id == k->id()) {
      if (k != this->skin_) {
        this->skin_ = k;
        ESP_LOGI(TAG, "Skin: %s", k->id());
      }
      this->dirty_full_ = true;
      this->dirty_live_ = false;
      this->loop();
      return true;
    }
  }
  ESP_LOGW(TAG, "Skin '%s' is not compiled in", id.c_str());
  return false;
}

const std::string &Note4UI::skin_id() const {
  // The header returns a reference; keep the last answer in a static.
  static std::string current;
  current = this->skin_ != nullptr ? this->skin_->id() : "";
  return current;
}

std::vector<std::string> Note4UI::skin_ids() const {
  std::vector<std::string> out;
  out.reserve(this->skins_.size());
  for (auto *k : this->skins_)
    out.emplace_back(k->id());
  return out;
}

void Note4UI::show(Screen s) {
  this->screen_ = s;
  this->dirty_full_ = true;
  this->dirty_live_ = false;
  this->loop();
}

void Note4UI::changed(bool live) {
  if (live) {
    // A pending full refresh already covers it.
    if (!this->dirty_full_)
      this->dirty_live_ = true;
  } else {
    // A full request supersedes pending partial ones.
    this->dirty_full_ = true;
    this->dirty_live_ = false;
  }
  this->loop();
}

void Note4UI::loop() {
  if (!this->dirty_full_ && !this->dirty_live_)
    return;
  if (!this->refresh_)
    return;  // keep the request until the driver hook is set
  const uint32_t now = millis();
  if (this->last_request_ms_ != 0 && now - this->last_request_ms_ < MIN_REQUEST_GAP_MS)
    return;  // coalesce; loop() picks it up later

  std::vector<Rect> rects;
  Refresh mode = Refresh::FULL_MONO;
  bool full = this->dirty_full_;
  if (!full) {
    if (this->skin_ != nullptr)
      rects = this->skin_->live_regions(this->screen_, this->model_);
    // A screen without live regions has nothing to refresh partially.
    if (rects.empty())
      full = true;
  }
  if (full) {
    rects.clear();
    bool gray = this->gray_enabled_ && this->skin_ != nullptr && this->skin_->gray_allowed(this->screen_) &&
                this->skin_->live_regions(this->screen_, this->model_).empty();
    if (gray && this->screen_ == Screen::ANSWER && answer_typing(this->model_))
      gray = false;
    mode = gray ? Refresh::FULL_GRAY : Refresh::FULL_MONO;
  } else {
    mode = Refresh::PARTIAL;
    // Clip to the panel; drop empty rects.
    std::vector<Rect> clipped;
    for (const auto &r : rects) {
      int x0 = std::max(0, r.x), y0 = std::max(0, r.y);
      int x1 = std::min(W, r.x + r.w), y1 = std::min(H, r.y + r.h);
      if (x1 > x0 && y1 > y0)
        clipped.push_back(Rect{x0, y0, x1 - x0, y1 - y0});
    }
    rects.swap(clipped);
    if (rects.empty()) {
      this->dirty_live_ = false;
      return;
    }
  }
  this->request_(mode, rects);
}

void Note4UI::request_(Refresh r, const std::vector<Rect> &rects) {
  this->dirty_full_ = false;
  this->dirty_live_ = false;
  ESP_LOGV(TAG, "Refresh: %s, %u rect(s)", refresh_name(r), (unsigned) rects.size());
  this->refresh_(r, rects);
  uint32_t t = millis();
  this->last_request_ms_ = t != 0 ? t : 1;
}

void Note4UI::draw(Display &d) {
  d.fill(paper());
  if (this->skin_ != nullptr)
    this->skin_->draw(d, this->screen_, this->model_);
}

}  // namespace note4_ui
}  // namespace esphome
