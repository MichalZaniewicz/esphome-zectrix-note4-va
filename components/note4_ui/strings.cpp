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
  const std::string d = (day == 1 && T.day_one != nullptr && T.day_one[0] != '\0') ? T.day_one : std::to_string(day);
  if (month < 1 || month > 12)
    return d;
  return subst(T.date_fmt, d, T.month_date[month - 1]);
}

// Every table, for reading names HA sent in any language. English first.
static const Strings *const *all_tables() {
  static const Strings *const ALL[6] = {&strings_en(), &strings_pl(), &strings_de(),
                                        &strings_nl(), &strings_fr(), &strings_es()};
  return ALL;
}
static const int TABLES = 6;

static bool starts_with(const std::string &s, const char *p) {
  const size_t n = p != nullptr ? strlen(p) : 0;
  return n > 0 && s.compare(0, n, p) == 0;
}

// The first word of `s`, lower case, without surrounding punctuation:
// " Wed., 18" -> "wed".
static std::string first_word(const std::string &s) {
  size_t a = s.find_first_not_of(" \t,.");
  if (a == std::string::npos)
    return "";
  size_t b = s.find_first_of(" \t,.0123456789", a);
  return to_lower(s.substr(a, b == std::string::npos ? std::string::npos : b - a));
}

int weekday_index(const std::string &name) {
  const std::string w = first_word(name);
  if (w.size() < 2)
    return -1;
  const Strings *const *all = all_tables();
  // 1. A full name ("dinsdag"), 2. a short form ("di", "mar", "Wed."),
  // 3. the start of a full name ("thurs"). In this order, because a short
  // form of one language can begin another language's name ("ma" / "martes").
  for (int k = 0; k < TABLES; k++)
    for (int i = 0; i < 7; i++)
      if (w == to_lower(all[k]->weekday[i]))
        return i;
  for (int k = 0; k < TABLES; k++)
    for (int i = 0; i < 7; i++)
      if (w == all[k]->weekday_short[i])
        return i;
  if (w.size() >= 3)
    for (int k = 0; k < TABLES; k++)
      for (int i = 0; i < 7; i++)
        if (starts_with(to_lower(all[k]->weekday[i]), w.c_str()))
          return i;
  return -1;
}

// One word -> 1..12, or 0. Every month_short is the start of its month's
// name in that language ("nov" -> "november", "listopada", "noviembre").
static int month_index(const std::string &word) {
  const std::string w = first_word(word);
  if (w.size() < 3)
    return 0;
  const Strings *const *all = all_tables();
  for (int k = 0; k < TABLES; k++)
    for (int i = 0; i < 12; i++)
      if (starts_with(w, all[k]->month_short[i]))
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
  // "17 November", "17. November", "17 de noviembre": a word after the
  // number (up to three are tried); "November 17": the one before it.
  std::string rest(end);
  for (int n = 0; n < 3 && month == 0; n++) {
    size_t a = rest.find_first_not_of(" \t,.");
    if (a == std::string::npos)
      break;
    size_t b = rest.find_first_of(" \t,.", a);
    month = month_index(rest.substr(a, b == std::string::npos ? std::string::npos : b - a));
    if (b == std::string::npos)
      break;
    rest = rest.substr(b);
  }
  if (month == 0 && p > 0)
    month = month_index(date.substr(0, p));
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

  set(s.now_kind, {"Window", "Storm", "Plant", "Vacuum", "School", "Flood", "Smoke", "Day", "Notice"});
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

static Strings make_de() {
  Strings s;
  s.code = "de";
  s.lowercase_names = false;
  s.decimal_sep = ',';

  set(s.weekday, {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"});
  set(s.weekday_short, {"so", "mo", "di", "mi", "do", "fr", "sa"});
  set(s.weekday_2, {"SO", "MO", "DI", "MI", "DO", "FR", "SA"});
  set(s.month_date, {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober",
                     "November", "Dezember"});
  set(s.month_short, {"jan", "feb", "mär", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "dez"});
  s.date_fmt = "{}. {}";
  set(s.condition, {"Klare Nacht", "Bewölkt", "Unwetter", "Nebel", "Hagel", "Gewitter", "Gewitter, Regen",
                    "Teils bewölkt", "Starkregen", "Regen", "Schnee", "Schneeregen", "Sonnig", "Windig",
                    "Windig, bewölkt"});

  s.next = "Als Nächstes";
  s.now = "Jetzt";
  s.alarm = "Alarm";
  s.today = "Heute";
  s.day = "Tag";
  s.tomorrow = "Morgen";
  s.weather = "Wetter";
  s.timer = "Timer";
  s.question = "Frage";
  s.heard = "Gehört";
  s.listening = "Ich höre";
  s.thinking = "Moment…";
  s.thinking_title = "Moment...";
  s.recording = "Aufnahme";
  s.assistant = "Assistent";
  s.answer = "Antwort";
  s.progress = "Fortschritt";
  s.conditions = "Wetterlage";
  s.time_label = "Zeit";
  s.clock_time = "Uhrzeit";
  s.sky = "Himmel";
  s.status = "Status";
  s.forecast = "Vorhersage";
  s.summary_label = "Info";
  s.attention = "Achtung!";
  s.buttons = "Tasten";
  s.wake_word = "Weckwort";
  s.room = "Raum";
  s.mic = "Mikro";
  s.mic_level = "Mikrofonpegel";
  s.after_silence = "Nach Stille";
  s.end = "Ende";
  s.total = "Gesamt";
  s.left = "Übrig";
  s.step_done = "Fertig";
  s.step_running = "Läuft…";
  s.details = "Details";
  s.more = "Mehr";
  s.answer_from_ha = "Antwort von Home Assistant";

  s.nothing_more = "Nichts mehr";
  s.in_calendar = "im Kalender";
  s.free_day = "Frei";
  s.nothing_planned = "nichts geplant";
  s.nothing_more_today = "Heute nichts mehr";
  s.no_events_1 = "Keine";
  s.no_events_2 = "Termine";

  s.speak_naturally = "Sprich einfach";
  s.stop_when_quiet = "Stille beendet die Aufnahme.";
  s.say = "Sag";
  s.and_ask = "und frag";

  s.quoted = "„{}“";
  s.say_wake = "Sag „{}“";
  s.of_total = "von {}";
  s.ends_at = "Ende {}";
  s.until = "bis {}";
  s.wake_word_is = "Weckwort: {}";
  s.error_fmt = "Etwas ist schiefgelaufen ({}).";

  set(s.steps, {"Sprache erkannt", "Frage Home Assistant", "Antwort wird erstellt"});
  set(s.steps_short, {"Sprache", "Home Assistant", "Antwort"});

  s.finish = "fertig";
  s.cancel = "abbrechen";
  s.stop = "stopp";
  s.pause = "pause";
  s.open_door = "öffnen";
  s.talk = "sprechen";
  s.close_door = "schließen";

  s.wind = "Wind";
  s.humidity = "Luftfeuchte";
  s.humidity_abbr = "LF";
  s.sunset = "Untergang";
  s.sunset_long = "Sonnenuntergang";
  s.sunset_abbr = "Unterg.";
  s.sunrise = "Aufgang";
  s.rain = "Regen";
  s.precip = "Regen";
  s.night = "Nacht";
  s.rain_per_hour = "Regen pro Std.";
  s.temperature = "Temperatur";
  s.rain_starts = "Regenbeginn";

  s.battery = "Akku";
  s.charging = "Lädt";
  s.charging_abbr = "Lad";
  s.without = "Ohne";
  s.no_wifi = "Kein WLAN";
  s.missing = "aus";

  s.doorbell = "Klingel";
  s.camera = "Kamera";
  s.just_now = "soeben";
  s.waiting_image = "Warte auf Bild";
  s.waiting_photo = "Warte auf Foto";
  s.ai_caption = "KI-Beschreibung";
  s.caption_source = "Text: AI Task";
  s.someone_rang = "Es hat geklingelt";
  s.someone_at_door = "Jemand ist an der Tür";
  s.default_place = "Gartentor";

  set(s.now_kind, {"Fenster", "Sturm", "Pflanze", "Sauger", "Schule", "Wasser", "Rauch", "Tag", "Hinweis"});
  set(s.now_topic, {"Zuhause", "Wetter", "Pflanzen", "Putzen", "Schule", "Wasser", "Rauch", "Zuhause", "Zuhause"});

  s.default_timer = "Timer";

  s.kw_rain = "regen|schirm";
  s.kw_precip = "niederschlag|schauer";
  s.kw_temp = "temp|max|höchst";
  return s;
}

static Strings make_nl() {
  Strings s;
  s.code = "nl";
  s.lowercase_names = true;
  s.decimal_sep = ',';

  set(s.weekday, {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"});
  set(s.weekday_short, {"zo", "ma", "di", "wo", "do", "vr", "za"});
  set(s.weekday_2, {"ZO", "MA", "DI", "WO", "DO", "VR", "ZA"});
  set(s.month_date, {"januari", "februari", "maart", "april", "mei", "juni", "juli", "augustus", "september",
                     "oktober", "november", "december"});
  set(s.month_short, {"jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"});
  set(s.condition, {"Heldere nacht", "Bewolkt", "Noodweer", "Mist", "Hagel", "Onweer", "Onweer, regen",
                    "Half bewolkt", "Stortregen", "Regen", "Sneeuw", "Natte sneeuw", "Zonnig", "Winderig",
                    "Winderig, bewolkt"});

  s.next = "Volgende";
  s.now = "Nu";
  s.alarm = "Alarm";
  s.today = "Vandaag";
  s.day = "Dag";
  s.tomorrow = "Morgen";
  s.weather = "Weer";
  s.timer = "Timer";
  s.question = "Vraag";
  s.heard = "Gehoord";
  s.listening = "Ik luister";
  s.thinking = "Momentje…";
  s.thinking_title = "Momentje...";
  s.recording = "Opname";
  s.assistant = "Assistent";
  s.answer = "Antwoord";
  s.progress = "Voortgang";
  s.conditions = "Weer nu";
  s.time_label = "Tijd";
  s.clock_time = "Tijd";
  s.sky = "Lucht";
  s.status = "Status";
  s.forecast = "Verwachting";
  s.summary_label = "Info";
  s.attention = "Let op!";
  s.buttons = "Knoppen";
  s.wake_word = "Wekwoord";
  s.room = "Kamer";
  s.mic = "Micro";
  s.mic_level = "Microniveau";
  s.after_silence = "Na stilte";
  s.end = "Einde";
  s.total = "Totaal";
  s.left = "Over";
  s.step_done = "Klaar";
  s.step_running = "Bezig…";
  s.details = "Details";
  s.more = "Meer";
  s.answer_from_ha = "Antwoord van Home Assistant";

  s.nothing_more = "Verder niets";
  s.in_calendar = "in de agenda";
  s.free_day = "Vrij";
  s.nothing_planned = "niets gepland";
  s.nothing_more_today = "Verder niets meer";
  s.no_events_1 = "Geen";
  s.no_events_2 = "afspraken";

  s.speak_naturally = "Spreek gewoon";
  s.stop_when_quiet = "Stilte stopt de opname.";
  s.say = "Zeg";
  s.and_ask = "en vraag";

  s.quoted = "“{}”";
  s.say_wake = "Zeg “{}”";
  s.of_total = "van {}";
  s.ends_at = "eindigt {}";
  s.until = "tot {}";
  s.wake_word_is = "Wekwoord: {}";
  s.error_fmt = "Er ging iets mis ({}).";

  set(s.steps, {"Spraak herkend", "Vraag Home Assistant", "Antwoord opstellen"});
  set(s.steps_short, {"spraak", "Home Assistant", "antwoord"});

  s.finish = "klaar";
  s.cancel = "annuleren";
  s.stop = "stop";
  s.pause = "pauze";
  s.open_door = "openen";
  s.talk = "praten";
  s.close_door = "sluiten";

  s.wind = "Wind";
  s.humidity = "Vochtigheid";
  s.humidity_abbr = "RV";
  s.sunset = "Zon onder";
  s.sunset_long = "Zonsondergang";
  s.sunset_abbr = "zon onder";
  s.sunrise = "Zon op";
  s.rain = "Regen";
  s.precip = "Neerslag";
  s.night = "nacht";
  s.rain_per_hour = "regen per uur";
  s.temperature = "temperatuur";
  s.rain_starts = "begin regen";

  s.battery = "Batterij";
  s.charging = "Laden";
  s.charging_abbr = "Laad";
  s.without = "Geen";
  s.no_wifi = "Geen wifi";
  s.missing = "uit";

  s.doorbell = "Deurbel";
  s.camera = "Camera";
  s.just_now = "zojuist";
  s.waiting_image = "Wachten op beeld";
  s.waiting_photo = "Wachten op foto";
  s.ai_caption = "AI-beschrijving";
  s.caption_source = "Tekst: AI Task";
  s.someone_rang = "Er werd aangebeld";
  s.someone_at_door = "Er staat iemand voor de deur";
  s.default_place = "Poort";

  set(s.now_kind, {"Raam", "Storm", "Plant", "Stofzuiger", "School", "Lekkage", "Rook", "Dag", "Melding"});
  set(s.now_topic, {"huis", "weer", "planten", "schoonmaak", "school", "lekkage", "rook", "huis", "huis"});

  s.default_timer = "Timer";

  s.kw_rain = "regen|paraplu";
  s.kw_precip = "neerslag|bui";
  s.kw_temp = "temp|max";
  return s;
}

static Strings make_fr() {
  Strings s;
  s.code = "fr";
  s.lowercase_names = true;
  s.decimal_sep = ',';

  set(s.weekday, {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"});
  set(s.weekday_short, {"dim", "lun", "mar", "mer", "jeu", "ven", "sam"});
  set(s.weekday_2, {"DI", "LU", "MA", "ME", "JE", "VE", "SA"});
  set(s.month_date, {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre",
                     "octobre", "novembre", "décembre"});
  set(s.month_short, {"janv", "févr", "mars", "avr", "mai", "juin", "juil", "août", "sept", "oct", "nov", "déc"});
  s.day_one = "1er";
  set(s.condition, {"Nuit claire", "Nuageux", "Alerte météo", "Brouillard", "Grêle", "Orage", "Orage, pluie",
                    "Éclaircies", "Forte pluie", "Pluie", "Neige", "Pluie et neige", "Ensoleillé", "Venteux",
                    "Venteux, nuageux"});

  s.next = "À suivre";
  s.now = "Info";
  s.alarm = "Alerte";
  s.today = "Aujourd'hui";
  s.day = "Jour";
  s.tomorrow = "Demain";
  s.weather = "Météo";
  s.timer = "Minuteur";
  s.question = "Question";
  s.heard = "J'ai entendu";
  s.listening = "J'écoute";
  s.thinking = "Un instant…";
  s.thinking_title = "Un instant...";
  s.recording = "Écoute";
  s.assistant = "Assistant";
  s.answer = "Réponse";
  s.progress = "Progression";
  s.conditions = "Conditions";
  s.time_label = "Heure";
  s.clock_time = "Heure";
  s.sky = "Ciel";
  s.status = "État";
  s.forecast = "Prévisions";
  s.summary_label = "Info";
  s.attention = "Attention !";
  s.buttons = "Boutons";
  s.wake_word = "Mot d'éveil";
  s.room = "Pièce";
  s.mic = "Micro";
  s.mic_level = "Niveau micro";
  s.after_silence = "Après silence";
  s.end = "Fin";
  s.total = "Total";
  s.left = "Reste";
  s.step_done = "Fait";
  s.step_running = "En cours…";
  s.details = "Détails";
  s.more = "Plus";
  s.answer_from_ha = "Réponse de Home Assistant";

  s.nothing_more = "Rien d'autre";
  s.in_calendar = "dans l'agenda";
  s.free_day = "Libre";
  s.nothing_planned = "rien de prévu";
  s.nothing_more_today = "Journée libre";
  s.no_events_1 = "Aucun";
  s.no_events_2 = "événement";

  s.speak_naturally = "Parlez librement";
  s.stop_when_quiet = "Je m'arrête au silence.";
  s.say = "Dites";
  s.and_ask = "et demandez";

  s.quoted = "« {} »";
  s.say_wake = "Dites « {} »";
  s.of_total = "sur {}";
  s.ends_at = "fin {}";
  s.until = "jusqu'à {}";
  s.wake_word_is = "Mot d'éveil : {}";
  s.error_fmt = "Un problème est survenu ({}).";

  set(s.steps, {"Parole reconnue", "Question à Home Assistant", "Rédaction de la réponse"});
  set(s.steps_short, {"parole", "Home Assistant", "réponse"});

  s.finish = "terminer";
  s.cancel = "annuler";
  s.stop = "stop";
  s.pause = "pause";
  s.open_door = "ouvrir";
  s.talk = "parler";
  s.close_door = "fermer";

  s.wind = "Vent";
  s.humidity = "Humidité";
  s.humidity_abbr = "hum";
  s.sunset = "Coucher";
  s.sunset_long = "Coucher";
  s.sunset_abbr = "coucher";
  s.sunrise = "Lever";
  s.rain = "Pluie";
  s.precip = "Pluie";
  s.night = "nuit";
  s.rain_per_hour = "pluie par heure";
  s.temperature = "température";
  s.rain_starts = "début pluie";

  s.battery = "Batterie";
  s.charging = "En charge";
  s.charging_abbr = "Charg";
  s.without = "Sans";
  s.no_wifi = "Pas de wifi";
  s.missing = "non";

  s.doorbell = "Sonnette";
  s.camera = "Caméra";
  s.just_now = "à l'instant";
  s.waiting_image = "En attente d'image";
  s.waiting_photo = "En attente de photo";
  s.ai_caption = "légende IA";
  s.caption_source = "Légende : AI Task";
  s.someone_rang = "On a sonné";
  s.someone_at_door = "Quelqu'un est à la porte";
  s.default_place = "Portail";

  set(s.now_kind, {"Fenêtre", "Alerte", "Plante", "Robot", "École", "Fuite", "Fumée", "Jour", "Avis"});
  set(s.now_topic, {"maison", "météo", "plantes", "ménage", "école", "fuite", "fumée", "maison", "maison"});

  s.default_timer = "Minuteur";

  s.kw_rain = "pluie|parapluie";
  s.kw_precip = "précip|averse";
  s.kw_temp = "temp|max";
  return s;
}

static Strings make_es() {
  Strings s;
  s.code = "es";
  s.lowercase_names = true;
  s.decimal_sep = ',';

  set(s.weekday, {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"});
  set(s.weekday_short, {"dom", "lun", "mar", "mié", "jue", "vie", "sáb"});
  set(s.weekday_2, {"DO", "LU", "MA", "MI", "JU", "VI", "SÁ"});
  set(s.month_date, {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre",
                     "octubre", "noviembre", "diciembre"});
  set(s.month_short, {"ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"});
  s.date_fmt = "{} de {}";
  set(s.condition, {"Noche despejada", "Nublado", "Tiempo extremo", "Niebla", "Granizo", "Tormenta",
                    "Tormenta, lluvia", "Parcialmente nublado", "Aguacero", "Lluvia", "Nieve", "Aguanieve", "Soleado",
                    "Ventoso", "Ventoso, nublado"});

  s.next = "Próximo";
  s.now = "Ahora";
  s.alarm = "Alarma";
  s.today = "Hoy";
  s.day = "Día";
  s.tomorrow = "Mañana";
  s.weather = "Tiempo";
  s.timer = "Temporizador";
  s.question = "Pregunta";
  s.heard = "He oído";
  s.listening = "Escucho";
  s.thinking = "Pensando…";
  s.thinking_title = "Pensando...";
  s.recording = "Grabando";
  s.assistant = "Asistente";
  s.answer = "Respuesta";
  s.progress = "Progreso";
  s.conditions = "Condiciones";
  s.time_label = "Hora";
  s.clock_time = "Hora";
  s.sky = "Cielo";
  s.status = "Estado";
  s.forecast = "Previsión";
  s.summary_label = "Info";
  s.attention = "¡Atención!";
  s.buttons = "Botones";
  s.wake_word = "Palabra clave";
  s.room = "Habitación";
  s.mic = "Micro";
  s.mic_level = "Nivel del micro";
  s.after_silence = "Tras silencio";
  s.end = "Fin";
  s.total = "Total";
  s.left = "Queda";
  s.step_done = "Hecho";
  s.step_running = "En curso…";
  s.details = "Detalles";
  s.more = "Más";
  s.answer_from_ha = "Respuesta de Home Assistant";

  s.nothing_more = "Nada más";
  s.in_calendar = "en el calendario";
  s.free_day = "Libre";
  s.nothing_planned = "nada previsto";
  s.nothing_more_today = "Nada más hoy";
  s.no_events_1 = "Sin";
  s.no_events_2 = "eventos";

  s.speak_naturally = "Habla normal";
  s.stop_when_quiet = "Termino cuando calles.";
  s.say = "Di";
  s.and_ask = "y pregunta";

  s.quoted = "«{}»";
  s.say_wake = "Di «{}»";
  s.of_total = "de {}";
  s.ends_at = "termina {}";
  s.until = "hasta las {}";
  s.wake_word_is = "Palabra clave: {}";
  s.error_fmt = "Algo ha salido mal ({}).";

  set(s.steps, {"Voz reconocida", "Consulto Home Assistant", "Preparo la respuesta"});
  set(s.steps_short, {"voz", "Home Assistant", "respuesta"});

  s.finish = "terminar";
  s.cancel = "cancelar";
  s.stop = "parar";
  s.pause = "pausa";
  s.open_door = "abrir";
  s.talk = "hablar";
  s.close_door = "cerrar";

  s.wind = "Viento";
  s.humidity = "Humedad";
  s.humidity_abbr = "hum";
  s.sunset = "Atardecer";
  s.sunset_long = "Puesta de sol";
  s.sunset_abbr = "ocaso";
  s.sunrise = "Amanecer";
  s.rain = "Lluvia";
  s.precip = "Lluvia";
  s.night = "noche";
  s.rain_per_hour = "lluvia por hora";
  s.temperature = "temperatura";
  s.rain_starts = "inicio lluvia";

  s.battery = "Batería";
  s.charging = "Cargando";
  s.charging_abbr = "Carg";
  s.without = "Sin";
  s.no_wifi = "Sin wifi";
  s.missing = "no";

  s.doorbell = "Timbre";
  s.camera = "Cámara";
  s.just_now = "ahora mismo";
  s.waiting_image = "Esperando imagen";
  s.waiting_photo = "Esperando foto";
  s.ai_caption = "descripción IA";
  s.caption_source = "Texto: AI Task";
  s.someone_rang = "Han llamado";
  s.someone_at_door = "Hay alguien en la puerta";
  s.default_place = "Puerta";

  set(s.now_kind, {"Ventana", "Aviso", "Planta", "Robot", "Colegio", "Fuga", "Humo", "Día", "Aviso"});
  set(s.now_topic, {"casa", "tiempo", "plantas", "limpieza", "colegio", "fuga", "humo", "casa", "casa"});

  s.default_timer = "Temporizador";

  s.kw_rain = "lluvia|paraguas";
  s.kw_precip = "precip|chubasco";
  s.kw_temp = "temp|máx|max";
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

const Strings &strings_de() {
  static const Strings s = make_de();
  return s;
}

const Strings &strings_nl() {
  static const Strings s = make_nl();
  return s;
}

const Strings &strings_fr() {
  static const Strings s = make_fr();
  return s;
}

const Strings &strings_es() {
  static const Strings s = make_es();
  return s;
}

const Strings *strings_for(const std::string &code) {
  for (int k = 0; k < TABLES; k++)
    if (code == all_tables()[k]->code)
      return all_tables()[k];
  return nullptr;
}

}  // namespace note4_ui
}  // namespace esphome
