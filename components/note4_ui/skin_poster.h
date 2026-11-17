// Skin "poster": poster geometry. A big black circle with the clock, quarter
// circles as background, black bars. Archivo Black + Archivo.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

class PosterSkin : public Skin {
 public:
  // clock: Archivo Black 56 (digits), huge: 118 (digits), timer: 44 (digits),
  // num: 30 (digits), head: Archivo Black 22, val: Archivo Black 14,
  // body: Archivo 19, bold: Archivo 700 13, small: Archivo 600 12, label: Archivo 700 10.
  PosterSkin(Font *clock, Font *huge, Font *timer, Font *num, Font *head, Font *val, Font *body, Font *bold,
             Font *small, Font *label)
      : f_clock_(clock),
        f_huge_(huge),
        f_timer_(timer),
        f_num_(num),
        f_head_(head),
        f_val_(val),
        f_body_(body),
        f_bold_(bold),
        f_small_(small),
        f_label_(label) {}

  const char *id() const override { return "poster"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void idle_(Display &d, const Model &m);
  void now_card_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void doorbell_(Display &d, const Model &m);
  int answer_lines_(const Model &m) const;

  Font *f_clock_, *f_huge_, *f_timer_, *f_num_, *f_head_, *f_val_, *f_body_, *f_bold_, *f_small_, *f_label_;
};

}  // namespace note4_ui
}  // namespace esphome
