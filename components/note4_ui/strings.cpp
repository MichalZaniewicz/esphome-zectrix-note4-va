#include "strings.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

namespace esphome {
namespace note4_ui {

// ---------------------------------------------------------------------------
// UTF-8 case mapping

static uint32_t decode(const std::string &s, size_t &i) {
  const uint8_t c0 = (uint8_t) s[i];
  int len = c0 < 0x80 ? 1 : (c0 >> 5) == 0x6 ? 2 : (c0 >> 4) == 0xE ? 3 : (c0 >> 3) == 0x1E ? 4 : 0;
  if (len == 0 || i + len > s.size()) {
    i++;
    return '?';
  }
  uint32_t cp = len == 1 ? c0 : len == 2 ? (c0 & 0x1F) : len == 3 ? (c0 & 0x0F) : (c0 & 0x07);
  for (int k = 1; k < len; k++)
    cp = (cp << 6) | ((uint8_t) s[i + k] & 0x3F);
  i += len;
  return cp;
}

static void encode(uint32_t cp, std::string &out) {
  if (cp < 0x80) {
    out += (char) cp;
  } else if (cp < 0x800) {
    out += (char) (0xC0 | (cp >> 6));
    out += (char) (0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += (char) (0xE0 | (cp >> 12));
    out += (char) (0x80 | ((cp >> 6) & 0x3F));
    out += (char) (0x80 | (cp & 0x3F));
  } else {
    out += (char) (0xF0 | (cp >> 18));
    out += (char) (0x80 | ((cp >> 12) & 0x3F));
    out += (char) (0x80 | ((cp >> 6) & 0x3F));
    out += (char) (0x80 | (cp & 0x3F));
  }
}

static uint32_t cp_upper(uint32_t c) {
  if (c >= 'a' && c <= 'z')
    return c - 32;
  if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
    return c - 32;
  // Latin Extended-A: pairs (upper even) 0100-0137 and 014A-0177,
  // (upper odd) 0139-0148 and 0179-017E.
  if ((c >= 0x100 && c <= 0x137) || (c >= 0x14A && c <= 0x177))
    return (c & 1) ? c - 1 : c;
  if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
    return (c & 1) ? c : c - 1;
  return c;
}

static uint32_t cp_lower(uint32_t c) {
  if (c >= 'A' && c <= 'Z')
    return c + 32;
  if (c >= 0xC0 && c <= 0xDE && c != 0xD7)
    return c + 32;
  if ((c >= 0x100 && c <= 0x137) || (c >= 0x14A && c <= 0x177))
    return (c & 1) ? c : c + 1;
  if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
    return (c & 1) ? c + 1 : c;
  return c;
}

static std::string map_all(const std::string &s, uint32_t (*fn)(uint32_t)) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size();)
    encode(fn(decode(s, i)), out);
  return out;
}

static std::string map_first(const std::string &s, uint32_t (*fn)(uint32_t)) {
  if (s.empty())
    return s;
  size_t i = 0;
  uint32_t c = decode(s, i);
  std::string out;
  encode(fn(c), out);
  out += s.substr(i);
  return out;
}

std::string to_upper(const std::string &s) { return map_all(s, cp_upper); }
std::string to_lower(const std::string &s) { return map_all(s, cp_lower); }
std::string to_cap_first(const std::string &s) { return map_first(s, cp_upper); }
std::string to_lower_first(const std::string &s) { return map_first(s, cp_lower); }

// ---------------------------------------------------------------------------
// Patterns and keywords

static std::string subst_list(const char *pattern, std::initializer_list<const std::string *> args) {
  std::string out;
  if (pattern == nullptr)
    return out;
  auto it = args.begin();
  for (const char *p = pattern; *p != '\0'; p++) {
    if (p[0] == '{' && p[1] == '}') {
      if (it != args.end()) {
        out += **it;
        ++it;
      }
      p++;
    } else {
      out += *p;
    }
  }
  return out;
}

std::string subst(const char *pattern, const std::string &a) { return subst_list(pattern, {&a}); }
std::string subst(const char *pattern, const std::string &a, const std::string &b) {
  return subst_list(pattern, {&a, &b});
}

bool contains_any(const std::string &s, const char *keywords) {
  if (keywords == nullptr)
    return false;
  const char *p = keywords;
  while (*p != '\0') {
    const char *e = strchr(p, '|');
    size_t n = e != nullptr ? (size_t) (e - p) : strlen(p);
    if (n > 0 && s.find(std::string(p, n)) != std::string::npos)
      return true;
    if (e == nullptr)
      break;
    p = e + 1;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Lookups

static const char *const CONDITION_KEYS[15] = {
    "clear-night", "cloudy", "exceptional", "fog", "hail", "lightning", "lightning-rainy", "partlycloudy",
    "pouring", "rainy", "snowy", "snowy-rainy", "sunny", "windy", "windy-variant"};

const char *condition_text(const Strings &T, const std::string &ha_condition) {
  for (int i = 0; i < 15; i++)
    if (ha_condition == CONDITION_KEYS[i])
      return T.condition[i] != nullptr ? T.condition[i] : "";
  return "";
}

int now_icon_index(const std::string &icon) {
  static const char *const ICONS[8] = {"window", "wind", "plant", "vacuum", "school", "flood", "smoke", "brief"};
  for (int i = 0; i < 8; i++)
    if (icon == ICONS[i])
      return i;
  return 8;
}

std::string format_date(const Strings &T, int day, int month) {
  if (month < 1 || month > 12)
    return std::to_string(day);
  return std::to_string(day) + " " + T.month_date[month - 1];
}

static bool starts_with(const std::string &s, const std::string &p) { return !p.empty() && s.compare(0, p.size(), p) == 0; }

int weekday_index(const std::string &name) {
  const std::string w = to_lower(name);
  if (w.size() < 2)
    return -1;
  for (const Strings *T : {&strings_en(), &strings_pl()}) {
    for (int i = 0; i < 7; i++) {
      // "Wed." starts with the short form; "Wednesday" and "wed" are prefixes of the full name.
      if (starts_with(w, T->weekday_short[i]) || starts_with(to_lower(T->weekday[i]), w))
        return i;
    }
  }
  return -1;
}

static int month_index(const std::string &word) {
  const std::string w = to_lower(word);
  for (const Strings *T : {&strings_en(), &strings_pl()})
    for (int i = 0; i < 12; i++)
      if (starts_with(w, T->month_short[i]))
        return i + 1;
  return 0;
}

bool parse_date(const std::string &date, int &day, int &month) {
  day = 0;
  month = 0;
  size_t p = date.find_first_of("0123456789");
  if (p == std::string::npos)
    return false;
  char *end = nullptr;
  long d = strtol(date.c_str() + p, &end, 10);
  if (d < 1 || d > 31)
    return false;
  day = (int) d;
  // "17 November": the word after the number; "November 17": the one before.
  std::string after(end);
  size_t a = after.find_first_not_of(" .,");
  if (a != std::string::npos)
    month = month_index(after.substr(a));
  if (month == 0 && p > 0)
    month = month_index(date.substr(date.find_first_not_of(" ")));
  return true;
}

// ---------------------------------------------------------------------------
// Tables

template<size_t N> static void set(const char *(&dst)[N], std::initializer_list<const char *> src) {
  size_t i = 0;
  for (const char *s : src)
    if (i < N)
      dst[i++] = s;
  while (i < N)
    dst[i++] = "";
}

static Strings make_en() {
  Strings s;
  s.code = "en";
  s.lowercase_names = false;
  s.decimal_sep = '.';

  set(s.weekday, {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
  set(s.weekday_short, {"sun", "mon", "tue", "wed", "thu", "fri", "sat"});
  set(s.weekday_2, {"SU", "MO", "TU", "WE", "TH", "FR", "SA"});
  set(s.month_date, {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October",
                     "November", "December"});
  set(s.month_short, {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"});
  set(s.condition, {"Clear night", "Cloudy", "Severe weather", "Fog", "Hail", "Thunderstorm", "Thunderstorm, rain",
                    "Partly cloudy", "Pouring rain", "Rain", "Snow", "Sleet", "Sunny", "Windy", "Windy, cloudy"});

  s.next = "Next";
  s.now = "Now";
  s.alarm = "Alarm";
  s.today = "Today";
  s.day = "Day";
  s.tomorrow = "Tomorrow";
  s.weather = "Weather";
  s.timer = "Timer";
  s.question = "Question";
  s.heard = "I heard";
  s.listening = "Listening";
  s.thinking = "Thinking…";
  s.thinking_title = "Thinking...";
  s.recording = "Recording";
  s.assistant = "Assistant";
  s.answer = "Answer";
  s.progress = "Progress";
  s.conditions = "Conditions";
  s.time_label = "Time";
  s.clock_time = "Time";
  s.sky = "Sky";
  s.status = "Status";
  s.forecast = "Forecast";
  s.summary_label = "Info";
  s.attention = "Alert!";
  s.buttons = "Buttons";
  s.wake_word = "Wake word";
  s.room = "Room";
  s.mic = "Mic";
  s.mic_level = "Mic level";
  s.after_silence = "After silence";
  s.end = "End";
  s.total = "Total";
  s.left = "Left";
  s.step_done = "Done";
  s.step_running = "Working…";
  s.details = "Details";
  s.more = "More";
  s.answer_from_ha = "Answer from Home Assistant";

  s.nothing_more = "Nothing else";
  s.in_calendar = "on the calendar";
  s.free_day = "Free";
  s.nothing_planned = "nothing planned";
  s.nothing_more_today = "Nothing else today";
  s.no_events_1 = "No";
  s.no_events_2 = "events";

  s.speak_naturally = "Speak naturally";
  s.stop_when_quiet = "I'll stop when you're quiet.";
  s.say = "Say";
  s.and_ask = "and ask";

  s.quoted = "“{}”";
  s.say_wake = "Say “{}”";
  s.of_total = "of {}";
  s.ends_at = "ends {}";
  s.until = "until {}";
  s.wake_word_is = "Wake word: {}";
  s.error_fmt = "Something went wrong ({}).";

  set(s.steps, {"Speech recognised", "Asking Home Assistant", "Writing the answer"});
  set(s.steps_short, {"speech", "Home Assistant", "answer"});

  s.finish = "done";
  s.cancel = "cancel";
  s.stop = "stop";
  s.pause = "pause";
  s.open_door = "open";
  s.talk = "talk";
  s.close_door = "close";

  s.wind = "Wind";
  s.humidity = "Humidity";
  s.humidity_abbr = "hum";
  s.sunset = "Sunset";
  s.sunset_long = "Sunset";
  s.sunset_abbr = "sunset";
  s.sunrise = "Sunrise";
  s.rain = "Rain";
  s.precip = "Rain";
  s.night = "night";
  s.rain_per_hour = "rain per hour";
  s.temperature = "temperature";
  s.rain_starts = "rain starts";

  s.battery = "Battery";
  s.charging = "Charging";
  s.charging_abbr = "Chg";
  s.without = "No";
  s.no_wifi = "No wifi";
  s.missing = "off";

  s.doorbell = "Doorbell";
  s.camera = "Camera";
  s.just_now = "just now";
  s.waiting_image = "Waiting for image";
  s.waiting_photo = "Waiting for photo";
  s.ai_caption = "AI photo caption";
  s.caption_source = "Caption: AI Task";
  s.someone_rang = "Someone rang";
  s.someone_at_door = "Someone is at the door";
  s.default_place = "Gate";

  set(s.now_kind, {"Window", "Warning", "Plant", "Vacuum", "School", "Flood", "Smoke", "Day", "Notice"});
  set(s.now_topic, {"home", "weather", "plants", "cleaning", "school", "flood", "smoke", "home", "home"});

  s.default_timer = "Timer";

  s.kw_rain = "rain|umbrella";
  s.kw_precip = "precip|shower";
  s.kw_temp = "temp|max|high";
  return s;
}

static Strings make_pl() {
  Strings s;
  s.code = "pl";
  s.lowercase_names = true;
  s.decimal_sep = ',';

  set(s.weekday, {"niedziela", "poniedziałek", "wtorek", "środa", "czwartek", "piątek", "sobota"});
  set(s.weekday_short, {"niedz", "pon", "wt", "śr", "czw", "pt", "sob"});
  set(s.weekday_2, {"ND", "PN", "WT", "ŚR", "CZ", "PT", "SB"});
  set(s.month_date, {"stycznia", "lutego", "marca", "kwietnia", "maja", "czerwca", "lipca", "sierpnia", "września",
                     "października", "listopada", "grudnia"});
  set(s.month_short, {"sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paź", "lis", "gru"});
  set(s.condition, {"Bezchmurnie", "Pochmurno", "Uwaga na pogodę", "Mgła", "Grad", "Burza", "Burza z deszczem",
                    "Częściowe zachmurzenie", "Ulewa", "Deszcz", "Śnieg", "Deszcz ze śniegiem", "Słonecznie",
                    "Wietrznie", "Wietrznie, pochmurno"});

  s.next = "Następne";
  s.now = "Teraz";
  s.alarm = "Alarm";
  s.today = "Dziś";
  s.day = "Dzień";
  s.tomorrow = "Jutro";
  s.weather = "Pogoda";
  s.timer = "Timer";
  s.question = "Pytanie";
  s.heard = "Usłyszałem";
  s.listening = "Słucham";
  s.thinking = "Chwila…";
  s.thinking_title = "Myślę...";
  s.recording = "Nagrywam";
  s.assistant = "Asystent";
  s.answer = "Odpowiedź";
  s.progress = "Postęp";
  s.conditions = "Warunki";
  s.time_label = "Czas";
  s.clock_time = "Godzina";
  s.sky = "Stan";
  s.status = "Stan";
  s.forecast = "Prognoza";
  s.summary_label = "Opis";
  s.attention = "Uwaga!";
  s.buttons = "Przyciski";
  s.wake_word = "Słowo";
  s.room = "Pokój";
  s.mic = "Mikrofon";
  s.mic_level = "Poziom mikrofonu";
  s.after_silence = "Po ciszy";
  s.end = "Koniec";
  s.total = "Całość";
  s.left = "Zostało";
  s.step_done = "Gotowe";
  s.step_running = "Trwa…";
  s.details = "Szczegóły";
  s.more = "Więcej";
  s.answer_from_ha = "Odpowiedź z Home Assistant";

  s.nothing_more = "Nic więcej";
  s.in_calendar = "w kalendarzu";
  s.free_day = "Wolne";
  s.nothing_planned = "nic w kalendarzu";
  s.nothing_more_today = "Nic więcej dziś";
  s.no_events_1 = "Brak";
  s.no_events_2 = "wydarzeń";

  s.speak_naturally = "Mów naturalnie";
  s.stop_when_quiet = "Skończę, gdy ucichniesz.";
  s.say = "Powiedz";
  s.and_ask = "i zapytaj";

  s.quoted = "„{}”";
  s.say_wake = "Powiedz „{}”";
  s.of_total = "z {}";
  s.ends_at = "koniec {}";
  s.until = "do {}";
  s.wake_word_is = "Słowo: {}";
  s.error_fmt = "Coś poszło nie tak ({}).";

  set(s.steps, {"Rozpoznano mowę", "Pytam Home Assistant", "Układam odpowiedź"});
  set(s.steps_short, {"mowa", "Home Assistant", "odpowiedź"});

  s.finish = "zakończ";
  s.cancel = "anuluj";
  s.stop = "stop";
  s.pause = "pauza";
  s.open_door = "otwórz";
  s.talk = "mów";
  s.close_door = "zamknij";

  s.wind = "Wiatr";
  s.humidity = "Wilgotność";
  s.humidity_abbr = "wilg";
  s.sunset = "Zachód";
  s.sunset_long = "Zachód słońca";
  s.sunset_abbr = "zach";
  s.sunrise = "Wschód";
  s.rain = "Deszcz";
  s.precip = "Opad";
  s.night = "noc";
  s.rain_per_hour = "opad co godzinę";
  s.temperature = "temperatura";
  s.rain_starts = "start deszczu";

  s.battery = "Bateria";
  s.charging = "Ładowanie";
  s.charging_abbr = "Ład";
  s.without = "Bez";
  s.no_wifi = "Brak wifi";
  s.missing = "brak";

  s.doorbell = "Dzwonek";
  s.camera = "Kamera";
  s.just_now = "przed chwilą";
  s.waiting_image = "Czekam na obraz";
  s.waiting_photo = "Czekam na zdjęcie";
  s.ai_caption = "opis AI ze zdjęcia";
  s.caption_source = "Opis: AI Task";
  s.someone_rang = "Ktoś dzwoni";
  s.someone_at_door = "Ktoś dzwoni do drzwi";
  s.default_place = "Furtka";

  set(s.now_kind, {"Okno", "Ostrzeżenie", "Roślina", "Odkurzacz", "Szkoła", "Zalanie", "Dym", "Dzień", "Uwaga"});
  set(s.now_topic, {"dom", "pogoda", "rośliny", "sprzątanie", "szkoła", "zalanie", "dym", "dom", "dom"});

  s.default_timer = "Minutnik";

  s.kw_rain = "deszcz|parasol";
  s.kw_precip = "opad";
  s.kw_temp = "temp|maks";
  return s;
}

const Strings &strings_en() {
  static const Strings s = make_en();
  return s;
}

const Strings &strings_pl() {
  static const Strings s = make_pl();
  return s;
}

const Strings *strings_for(const std::string &code) {
  if (code == "en")
    return &strings_en();
  if (code == "pl")
    return &strings_pl();
  return nullptr;
}

}  // namespace note4_ui
}  // namespace esphome
