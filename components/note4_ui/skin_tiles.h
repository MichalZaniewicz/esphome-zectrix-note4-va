// Skin "tiles" (style D): rounded tiles in several greys, big friendly
// Manrope numbers, pill buttons. Uses the 16 greys the most.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

struct TilesFonts {
  Font *clock{nullptr};   // 800 72
  Font *temp{nullptr};    // 800 44, temperature now
  Font *next{nullptr};    // 800 28, next event time / tomorrow numbers
  Font *card{nullptr};    // 800 32, timer tile digits, the "Listening" / "Thinking…" word
  Font *timer{nullptr};   // 800 48
  Font *fc_hi{nullptr};   // 800 60
  Font *fc_lo{nullptr};   // 800 22
  Font *answer{nullptr};  // 600 18
  Font *m15{nullptr};     // 800 15
  Font *m14{nullptr};     // 800 14
  Font *m14m{nullptr};    // 700 14
  Font *m13{nullptr};     // 800 13
  Font *m12{nullptr};     // 800 12
  Font *m12m{nullptr};    // 700 12
  Font *m11{nullptr};     // 800 11
  Font *m11m{nullptr};    // 700 11
  Font *m10{nullptr};     // 800 10
};

class TilesSkin : public Skin {
 public:
  explicit TilesSkin(const TilesFonts &f) : f_(f) {}
  const char *id() const override { return "tiles"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void status_(Display &d, const Model &m, const std::string &left);
  void pill_(Display &d, int x, int y, int w, int h, const std::string &label, bool dark, Font *f);
  void idle_(Display &d, const Model &m);
  void now_tile_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  TilesFonts f_;
};

}  // namespace note4_ui
}  // namespace esphome
