// Skin "piksel": retro handheld console. Pixel fonts, double-framed windows,
// pixel icons, time bars like a health bar. Press Start 2P + VT323, 1 bpp.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

class PikselSkin : public Skin {
 public:
  // s: Press Start 2P 8, m: PS2P 16, l: PS2P 32 (digits, °, C), xl: PS2P 48 (digits),
  // xxl: PS2P 56 (digits), vt: VT323 25.
  PikselSkin(Font *s, Font *m, Font *l, Font *xl, Font *xxl, Font *vt)
      : f_s_(s), f_m_(m), f_l_(l), f_xl_(xl), f_xxl_(xxl), f_vt_(vt) {}

  const char *id() const override { return "piksel"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void titlebar_(Display &d, const std::string &l, const std::string &r);
  void win_(Display &d, int x, int y, int w, int h, const std::string &title);
  void button_(Display &d, int x, int cy, char key, int r, const std::string &label, int label_dx, int label_dy);
  void idle_(Display &d, const Model &m);
  void now_win_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void doorbell_(Display &d, const Model &m);
  int answer_lines_(const Model &m) const;

  Font *f_s_, *f_m_, *f_l_, *f_xl_, *f_xxl_, *f_vt_;
};

}  // namespace note4_ui
}  // namespace esphome
