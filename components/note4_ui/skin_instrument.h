// Skin "instrument" (style C): a measuring device. Monospaced type, numbered
// fields, scales, a dot grid behind everything, a label for every number.
#pragma once

#include "note4_ui.h"

namespace esphome {
namespace note4_ui {

struct InstrumentFonts {
  // IBM Plex Mono
  Font *clock{nullptr};  // 600 78
  Font *next{nullptr};   // 600 26, next event time
  Font *card{nullptr};   // 600 28, timer module / tomorrow / doorbell time
  Font *fc_hi{nullptr};  // 600 54
  Font *timer{nullptr};  // 600 42
  Font *m20s{nullptr};   // 600 20, timer side values, elapsed time
  Font *m13s{nullptr};   // 600 13
  Font *m12s{nullptr};   // 600 12
  Font *m10s{nullptr};   // 600 10, values, status bar, keycaps
  Font *m10{nullptr};    // 400 10, labels
  Font *m9s{nullptr};    // 600 9, tags
  Font *m9{nullptr};     // 500 9, axis, footers
  Font *m8s{nullptr};    // 600 8, timeline names, small labels
  // IBM Plex Sans
  Font *answer{nullptr};  // 400 19
};

class InstrumentSkin : public Skin {
 public:
  explicit InstrumentSkin(const InstrumentFonts &f) : f_(f) {}
  const char *id() const override { return "instrument"; }
  void draw(Display &d, Screen s, const Model &m) override;
  std::vector<Rect> live_regions(Screen s, const Model &m) const override;

 protected:
  int answer_lines_(const Model &m) const;
  void frame_(Display &d, const Model &m, const std::string &left);
  void status_(Display &d, const Model &m, const std::string &left);
  int tag_(Display &d, const std::string &txt, int x, int y, bool inv = true, bool on_black = false);
  void module_(Display &d, int x, int w, const std::string &title);
  void idle_(Display &d, const Model &m);
  void now_module_(Display &d, const Model &m);
  void answer_(Display &d, const Model &m);
  void weather_(Display &d, const Model &m);
  void timer_(Display &d, const Model &m);
  void listen_(Display &d, const Model &m);
  void think_(Display &d, const Model &m);
  void doorbell_(Display &d, const Model &m);
  InstrumentFonts f_;
};

}  // namespace note4_ui
}  // namespace esphome
