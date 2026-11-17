// Skin "tarcza": like a good watch. Analogue dial with hands, date window,
// sub-dials for weather and timer, a 24 h dial for tomorrow.
// Barlow Condensed + Barlow.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

class TarczaSkin : public Skin {
 public:
  // num: Barlow Condensed 700 16, big: BC 700 26 (digits), huge: BC 700 32 (digits),
  // title: BC 700 20, head: BC 600 28, label: BC 600 11,
  // body: Barlow 500 19, text: Barlow 600 14, small: Barlow 600 12.
  TarczaSkin(Font *num, Font *big, Font *huge, Font *title, Font *head, Font *label, Font *body, Font *text,
             Font *small)
      : f_num_(num),
        f_big_(big),
        f_huge_(huge),
        f_title_(title),
        f_head_(head),
        f_label_(label),
        f_body_(body),
        f_text_(text),
        f_small_(small) {}

  const char *id() const override { return "tarcza"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void gauge_(Display &d, float cx, float cy, float r, float frac, const std::string &val, const std::string &label,
              bool live);
  void idle_(Display &d, const Model &m);
  void now_card_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void doorbell_(Display &d, const Model &m);
  int answer_lines_(const Model &m) const;

  Font *f_num_, *f_big_, *f_huge_, *f_title_, *f_head_, *f_label_, *f_body_, *f_text_, *f_small_;
};

}  // namespace note4_ui
}  // namespace esphome
