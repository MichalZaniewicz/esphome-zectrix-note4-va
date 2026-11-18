// Skin "swiss" (style B): strict grid, huge Inter Tight numerals, thick
// rules, no icons, one black block per screen.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

struct SwissFonts {
  // Inter Tight
  Font *clock{nullptr};     // 800 122, clock and timer digits
  Font *next{nullptr};      // 700 32, next event time, step numbers, seconds
  Font *title{nullptr};     // 800 28, "Now" title
  Font *card{nullptr};      // 800 34, timer card / tomorrow numbers
  Font *question{nullptr};  // 700 21
  Font *fact{nullptr};      // 800 30, answer key facts
  Font *fc_hi{nullptr};     // 800 104
  Font *fc_lo{nullptr};     // 700 38
  Font *h26{nullptr};       // 800 26, "Tomorrow", timer name, wake word
  Font *end{nullptr};       // 800 24, timer end time
  Font *big{nullptr};       // 800 56, the "Listening" / "Thinking…" word
  // Inter
  Font *i9b{nullptr};    // 700 9, labels
  Font *i10b{nullptr};   // 700 10, status bar, axis
  Font *i10s{nullptr};   // 600 10
  Font *i11b{nullptr};   // 700 11
  Font *i11s{nullptr};   // 600 11
  Font *i12{nullptr};    // 400 12
  Font *i12s{nullptr};   // 600 12
  Font *i12b{nullptr};   // 700 12, hints, values
  Font *i13s{nullptr};   // 600 13
  Font *i14b{nullptr};   // 700 14
  Font *i15s{nullptr};   // 600 15
  Font *i19{nullptr};    // 400 19, answer text
};

class SwissSkin : public Skin {
 public:
  explicit SwissSkin(const SwissFonts &f) : f_(f) {}
  const char *id() const override { return "swiss"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  int answer_lines_(const Model &m) const;
  void status_(Display &d, const Model &m, const std::string &left);
  void idle_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void doorbell_(Display &d, const Model &m);
  SwissFonts f_;
};

}  // namespace note4_ui
}  // namespace esphome
