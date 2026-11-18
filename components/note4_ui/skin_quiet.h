// Skin "quiet": an almost empty screen. One big thing, the rest whispered in
// grey. No frames, no icons, lots of air. Instrument Serif + Instrument Sans.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

class QuietSkin : public Skin {
 public:
  // huge: Instrument Serif 156 (digits), body: Instrument Serif 28,
  // italic: Instrument Serif italic 26, small: Instrument Sans 500 13,
  // tiny: Instrument Sans 500 11.
  QuietSkin(Font *huge, Font *body, Font *italic, Font *small, Font *tiny)
      : f_huge_(huge), f_body_(body), f_italic_(italic), f_small_(small), f_tiny_(tiny) {}

  const char *id() const override { return "quiet"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void idle_(Display &d, const Model &m);
  void now_col_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);

  Font *f_huge_, *f_body_, *f_italic_, *f_small_, *f_tiny_;
};

}  // namespace note4_ui
}  // namespace esphome
