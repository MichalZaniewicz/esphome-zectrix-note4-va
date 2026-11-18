// Skin "paper" (style A): Literata serif like an e-book reader, Atkinson
// Hyperlegible for labels, thin rounded frames, one negative card.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

struct PaperFonts {
  // Literata
  Font *clock{nullptr};      // 700, clock digits
  Font *date{nullptr};       // 400 italic 17, date line
  Font *temp{nullptr};       // 600 30, temperature now
  Font *next{nullptr};       // 700 24, next event time
  Font *brief{nullptr};      // 400 italic 13, day brief sentence
  Font *card{nullptr};       // 700 28, timer card / tomorrow card numbers
  Font *title{nullptr};      // 600 16, "Now" card title
  Font *question{nullptr};   // 400 italic 19
  Font *answer{nullptr};     // 400 22
  Font *fc_hi{nullptr};      // 700 58
  Font *fc_lo{nullptr};      // 400 26
  Font *timer{nullptr};      // 700 48, timer digits
  Font *times{nullptr};      // 600 22, timer start / end
  Font *big{nullptr};        // 500 italic 50, the "Listening" / "Thinking…" word
  Font *bell_time{nullptr};  // 700 34
  Font *caption{nullptr};    // 600 21, doorbell caption
  // Atkinson Hyperlegible
  Font *l9b{nullptr};    // 700 9, small caps labels
  Font *l10b{nullptr};   // 700 10, status bar, axis, footer
  Font *l10i{nullptr};   // 400 italic 10
  Font *l11{nullptr};    // 400 11
  Font *l11i{nullptr};   // 400 italic 11
  Font *l11b{nullptr};   // 700 11, high / low numbers
  Font *l12{nullptr};    // 400 12
  Font *l12b{nullptr};   // 700 12, keycaps, chart labels
  Font *l13{nullptr};    // 400 13
  Font *l13b{nullptr};   // 700 13
  Font *l14b{nullptr};   // 700 14
};

class PaperSkin : public Skin {
 public:
  explicit PaperSkin(const PaperFonts &f) : f_(f) {}
  const char *id() const override { return "paper"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  void status_(Display &d, const Model &m, const std::string &left, bool inv);
  void idle_(Display &d, const Model &m);
  void now_card_(Display &d, const Model &m, int x, int y, int w, int h);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m, bool thinking);
  void doorbell_(Display &d, const Model &m);
  PaperFonts f_;
};

}  // namespace note4_ui
}  // namespace esphome
