// On-screen language packs: every phrase the skins and the core draw, one
// table per language: en (default), pl, de, nl, fr, es. `language: xx` in the
// thin config picks one (Note4UI::set_language, called from core.yaml).
//
// Skins read the active table through the model: `const Strings &T = *m.tr;`
// then `T.next`, `T.weekday[i]`, `subst(T.of_total, mmss(...))`.
//
// Phrases are stored in sentence case ("Next", "cancel") and each skin
// applies its own case (upper(), cap_first()), exactly as it did with the
// literals before. The case helpers below work on ASCII, Latin-1 and Latin
// Extended-A, so accented letters upper-case correctly too.
//
// ADDING A LANGUAGE: copy make_en() in strings.cpp, translate every field,
// add it to strings_for(). Every glyph must exist in the skins' text fonts
// (GF_Latin_Core in base/skins/*.yaml); the few restricted fonts (the big
// "Listening" / "Thinking…" words, paper's l12) list their glyphs
// explicitly and need the new letters added.
#pragma once

#include <string>

namespace esphome {
namespace note4_ui {

struct Strings {
  const char *code = "";       // "en", "pl"
  bool lowercase_names = false;  // weekday names are lower case mid-sentence (true for pl)
  char decimal_sep = '.';      // "4.5 mm" / "4,5 mm"

  // ---- calendar. Index 0 = Sunday / January. ----
  const char *weekday[7] = {};       // as in a date line: "Tuesday"
  const char *weekday_short[7] = {};  // lower case: "tue"
  const char *weekday_2[7] = {};     // two-letter caps: "TU"
  const char *month_date[12] = {};   // after a day number: "November" (pl: genitive)
  const char *month_short[12] = {};  // lower case, for parsing dates from HA: "nov"
  const char *date_fmt = "{} {}";   // day, month: "17 November", "17. November", "17 de noviembre"
  const char *day_one = "";         // the 1st when it is not just "1": "1er" (fr)

  // ---- weather now, by HA condition (order: see CONDITION_KEYS in strings.cpp) ----
  const char *condition[15] = {};

  // ---- section labels ----
  const char *next = "";            // "Next"
  const char *now = "";             // "Now"
  const char *alarm = "";           // "Alarm"
  const char *today = "";           // "Today"
  const char *day = "";             // "Day"
  const char *tomorrow = "";        // "Tomorrow"
  const char *weather = "";         // "Weather"
  const char *timer = "";           // "Timer"
  const char *question = "";        // "Question"
  const char *heard = "";           // "I heard"
  const char *listening = "";       // "Listening" (big word)
  const char *thinking = "";        // "Thinking…" (big word)
  const char *thinking_title = "";  // pixel title bar: "Thinking..."
  const char *recording = "";       // "Recording"
  const char *assistant = "";       // "Assistant"
  const char *answer = "";          // "Answer"
  const char *progress = "";        // "Progress"
  const char *conditions = "";      // "Conditions"
  const char *time_label = "";      // "Time" (clock module)
  const char *sky = "";             // "Sky" (condition row)
  const char *status = "";          // "Status" (device status window)
  const char *forecast = "";        // "Forecast"
  const char *summary_label = "";   // "Info"
  const char *attention = "";       // "Alert!"
  const char *buttons = "";         // "Buttons"
  const char *wake_word = "";       // "Wake word"
  const char *room = "";            // "Room"
  const char *mic = "";             // "Mic"
  const char *mic_level = "";       // "Mic level"
  const char *after_silence = "";   // "After silence"
  const char *end = "";             // "End"
  const char *total = "";           // "Total"
  const char *left = "";            // "Left"
  const char *step_done = "";       // "Done"
  const char *step_running = "";    // "Working…"
  const char *details = "";         // "Details"
  const char *more = "";            // "More"
  const char *answer_from_ha = "";  // "Answer from Home Assistant"

  // ---- nothing on the calendar ----
  const char *nothing_more = "";        // "Nothing else"
  const char *in_calendar = "";         // "on the calendar"
  const char *free_day = "";            // "Free"
  const char *nothing_planned = "";     // "nothing planned"
  const char *nothing_more_today = "";  // "Nothing else today"
  const char *no_events_1 = "";         // "No"
  const char *no_events_2 = "";         // "events"

  // ---- listening, hints ----
  const char *speak_naturally = "";  // "Speak naturally" (no full stop)
  const char *stop_when_quiet = "";  // "I'll stop when you're quiet."
  const char *say = "";              // "Say"
  const char *and_ask = "";          // "and ask"

  // ---- patterns, "{}" = the argument (see subst()) ----
  const char *quoted = "";      // "“{}”"
  const char *say_wake = "";    // "Say “{}”"
  const char *of_total = "";    // "of {}"   (timer total)
  const char *ends_at = "";     // "ends {}" (timer end time)
  const char *until = "";       // "until {}"
  const char *wake_word_is = "";  // "Wake word: {}"
  const char *error_fmt = "";   // "Something went wrong ({})."

  // ---- the three pipeline steps on the THINK screen ----
  const char *steps[3] = {};        // "Speech recognised", "Asking Home Assistant", "Writing the answer"
  const char *steps_short[3] = {};  // "speech", "Home Assistant", "answer"

  // ---- button actions, lower case ----
  const char *finish = "";  // end the recording: "done"
  const char *cancel = "";
  const char *stop = "";
  const char *pause = "";

  // ---- weather words ----
  const char *wind = "";
  const char *humidity = "";
  const char *humidity_abbr = "";  // lower case, no full stop: "hum"
  const char *sunset = "";
  const char *sunset_long = "";    // "Sunset" (long form, paper)
  const char *sunset_abbr = "";    // lower case: "sunset" (pixel)
  const char *sunrise = "";
  const char *rain = "";           // rain start marker: "Rain"
  const char *precip = "";         // rain amount: "Rain"
  const char *night = "";          // dial legend, lower case
  const char *rain_per_hour = "";
  const char *temperature = "";
  const char *rain_starts = "";

  // ---- device state ----
  const char *battery = "";
  const char *charging = "";
  const char *charging_abbr = "";  // "Chg"
  const char *without = "";        // "No" Wi-Fi / HA
  const char *no_wifi = "";        // "No wifi"
  const char *missing = "";        // "wifi: off"

  // ---- the "Now" card, by Model::now_icon (index: now_icon_index()) ----
  const char *now_kind[9] = {};   // "Window", "Warning", ...
  const char *now_topic[9] = {};  // lower case: "home", "weather", ...

  // ---- core.yaml ----
  const char *default_timer = "";  // a timer HA sent without a name

  // ---- key facts: lower-case keywords, '|'-separated (see contains_any()) ----
  const char *kw_rain = "";
  const char *kw_precip = "";
  const char *kw_temp = "";
};

const Strings &strings_en();
const Strings &strings_pl();
const Strings &strings_de();
const Strings &strings_nl();
const Strings &strings_fr();
const Strings &strings_es();
// nullptr for an unknown code.
const Strings *strings_for(const std::string &code);

// "{}" in `pattern` replaced by `a` (first) and `b` (second).
std::string subst(const char *pattern, const std::string &a);
std::string subst(const char *pattern, const std::string &a, const std::string &b);

// UTF-8 case mapping: ASCII, Latin-1 Supplement, Latin Extended-A.
std::string to_upper(const std::string &s);
std::string to_lower(const std::string &s);
std::string to_cap_first(const std::string &s);
std::string to_lower_first(const std::string &s);

// `s` contains one of the '|'-separated keywords (case as given).
bool contains_any(const std::string &s, const char *keywords);

// HA condition keyword ("partlycloudy") -> text in T, "" when unknown.
const char *condition_text(const Strings &T, const std::string &ha_condition);
// Model::now_icon -> 0..8 (window, wind, plant, vacuum, school, flood, smoke, brief, anything else).
int now_icon_index(const std::string &icon);
// T.date_fmt filled in: "17 November", "17. November". month 1..12.
std::string format_date(const Strings &T, int day, int month);
// A weekday name in any of the languages ("Wednesday", "wed", "Wed.") ->
// 0 (Sunday) .. 6, or -1.
int weekday_index(const std::string &name);
// "17 November", "November 17", "17 Nov" -> day and month
// (1..12). False when no day number is found; month 0 when it has none.
bool parse_date(const std::string &date, int &day, int &month);

}  // namespace note4_ui
}  // namespace esphome
