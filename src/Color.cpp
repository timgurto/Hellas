#include "Color.h"

#include "util.h"

const Color Color::BLUE_HELL{0x18, 0x52, 0xa1};
const Color Color::NO_KEY{0x01, 0x02, 0x03};
const Color Color::DEBUG_TEXT = {0x00, 0xff, 0xff};
const Color Color::MISSING_IMAGE = BLUE_HELL;

const Color Color::BLACK(0x00, 0x00, 0x00);
const Color Color::BLUE(0x00, 0x00, 0xff);
const Color Color::GREEN(0x00, 0xff, 0x00);
const Color Color::CYAN(0x00, 0xff, 0xff);
const Color Color::RED(0xff, 0x00, 0x00);
const Color Color::MAGENTA(0xff, 0x00, 0xff);
const Color Color::YELLOW(0xff, 0xff, 0x00);
const Color Color::WHITE(0xff, 0xff, 0xff);

const Color Color::CHAT_BACKGROUND = BLACK;
const Color Color::CHAT_DEFAULT = YELLOW * 0.9;
const Color Color::CHAT_SAY{0xcc, 0xcc, 0xcc};
const Color Color::CHAT_WHISPER = {0xff, 0x99, 0x99};
const Color Color::CHAT_WARNING = YELLOW;
const Color Color::CHAT_ERROR = RED;
const Color Color::CHAT_SUCCESS = CHAT_DEFAULT;

const Color Color::WINDOW_BACKGROUND = WHITE * 0.1;
const Color Color::WINDOW_DARK = WHITE * 0.0;
const Color Color::WINDOW_LIGHT = WHITE * 0.2;
const Color Color::WINDOW_FONT{0xcc, 0xcc, 0xcc};
const Color Color::WINDOW_FLAVOUR_TEXT = RED * 0.5 + YELLOW * 0.5 + BLUE * 0.2;
const Color Color::WINDOW_HEADING = BLUE * .4 + CYAN * .6;

const Color Color::COMBATANT_SELF = GREEN * 0.8 + BLUE;
const Color Color::COMBATANT_ALLY = GREEN * 0.8;
const Color Color::COMBATANT_DEFENSIVE = YELLOW * 0.9;
const Color Color::COMBATANT_ENEMY = RED * 0.8 + WHITE * 0.2;
const Color Color::COMBATANT_NEUTRAL = WHITE * 0.9;

const Color Color::FOOTPRINT_GOOD = WHITE;
const Color Color::FOOTPRINT_BAD = RED;
const Color Color::FOOTPRINT_COLLISION = RED + GREEN * 0.5;
const Color Color::FOOTPRINT_ACTIVE = WHITE * 0.2;

const Color Color::TOOLTIP_BACKGROUND = WINDOW_BACKGROUND;
const Color Color::TOOLTIP_BORDER{0xa7, 0xa7, 0xa7};
const Color Color::TOOLTIP_NAME = WHITE;
const Color Color::TOOLTIP_BODY{0xa7, 0xa7, 0xa7};
const Color Color::TOOLTIP_TAG = GREEN * .7 + WHITE * .3;
const Color Color::TOOLTIP_INSTRUCTION = YELLOW * .8 + RED * .2;
const Color Color::TOOLTIP_FLAVOUR = WHITE * .5;
const Color Color::TOOLTIP_BAD = RED;

const Color Color::ITEM_QUALITY_COMMON = WHITE;
const Color Color::ITEM_QUALITY_UNCOMMON = GREEN * .9 + YELLOW * .1;
const Color Color::ITEM_QUALITY_RARE = BLUE * .4 + CYAN * .4;
const Color Color::ITEM_QUALITY_EPIC = MAGENTA * .4 + BLUE * .3 + WHITE * .2;
const Color Color::ITEM_QUALITY_LEGENDARY = RED * .5 + YELLOW * .5;

const Color Color::STAT_HEALTH = GREEN * .8;
const Color Color::STAT_ENERGY = RED + GREEN * .8;
const Color Color::STAT_XP = MAGENTA * .5 + CYAN * .1;
const Color Color::STAT_AIR = WHITE * .6 + MAGENTA * .2 + BLUE * .3;
const Color Color::STAT_EARTH = GREEN * .6 + RED * .1;
const Color Color::STAT_FIRE = RED + GREEN * .3;
const Color Color::STAT_WATER = CYAN * .5 + BLUE * .5;

const Color Color::FLOATING_CORE = BLACK;
const Color Color::FLOATING_DAMAGE = YELLOW;
const Color Color::FLOATING_HEAL = STAT_HEALTH;
const Color Color::FLOATING_MISS = BLUE + GREEN * .5;
const Color Color::FLOATING_CRIT = RED + YELLOW * .5;
const Color Color::FLOATING_INFO = YELLOW;

const Color Color::BUFF = WHITE;
const Color Color::DEBUFF = RED * 0.9 + WHITE * 0.1;

const Color Color::UI_OUTLINE = BLACK;
const Color Color::UI_OUTLINE_HIGHLIGHT = WHITE;
const Color Color::UI_FEEDBACK = YELLOW * 0.9;
const Color Color::UI_TEXT = WHITE;
const Color Color::UI_DISABLED = WHITE * 0.6;
const Color Color::UI_PROGRESS_BAR = WINDOW_LIGHT;

const Color Color::RELEASE_NOTES_VERSION = GREEN * .7 + WHITE * .3;
const Color Color::RELEASE_NOTES_SUBHEADING = YELLOW * .8 + RED * .2;
const Color Color::RELEASE_NOTES_BODY{0xcc, 0xcc, 0xcc};

const Color Color::CHANCE_SMALL = YELLOW * .4 + WHITE * .3;
const Color Color::CHANCE_MODERATE = GREEN * .5 + WHITE * .4;
const Color Color::CHANCE_HIGH = CYAN * .4 + BLUE * .2 + WHITE * .4;

const Color Color::SPRITE_OUTLINE = 0x330a17;
const Color Color::SPRITE_OUTLINE_HIGHLIGHT = 0xE5E5E5;

const Color Color::DURABILITY_LOW = YELLOW;
const Color Color::DURABILITY_BROKEN = RED;

const Color Color::DIFFICULTY_VERY_HIGH = RED;
const Color Color::DIFFICULTY_HIGH = RED * .5 + YELLOW * .5;
const Color Color::DIFFICULTY_NEUTRAL = YELLOW;
const Color Color::DIFFICULTY_LOW = GREEN * 0.75;
const Color Color::DIFFICULTY_VERY_LOW = WHITE * 0.75;

Color::Color(Uint8 r, Uint8 g, Uint8 b) : _r(r), _g(g), _b(b) {}

Color::Color(const SDL_Color &rhs) : _r(rhs.r), _g(rhs.g), _b(rhs.b) {}

Color::Color(Uint32 rhs) {
  Uint8 little = rhs % 0x100, middle = (rhs >> 8) % 0x100,
        big = (rhs >> 16) % 0x100;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  _r = big;
  _g = middle;
  _b = little;
#else
  _r = little;
  _g = middle;
  _b = big;
#endif
}

Color::operator SDL_Color() const {
  const SDL_Color c = {_r, _g, _b, 0};
  return c;
}

Color::operator Uint32() const {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  return (_r << 16) | (_g << 8) | (_b);
#else
  return (_r) | (_g << 8) | (_b << 16);
#endif
}

Color Color::operator/(double s) const {
  if (s < 0) return BLACK;
  int r = toInt(_r / s), g = toInt(_g / s), b = toInt(_b / s);
  if (r > 0xff) r = 0xff;
  if (g > 0xff) g = 0xff;
  if (b > 0xff) b = 0xff;
  return Color(r, g, b);
}

Color Color::operator/(int s) const { return *this / static_cast<double>(s); }

Color Color::operator*(double d) const {
  if (d < 0) return BLACK;
  int r = toInt(_r * d), g = toInt(_g * d), b = toInt(_b * d);
  if (r > 0xff) r = 0xff;
  if (g > 0xff) g = 0xff;
  if (b > 0xff) b = 0xff;
  return Color(r, g, b);
}

Color Color::operator*(int s) const { return *this * static_cast<double>(s); }

Color operator+(const Color &lhs, const Color &rhs) {
  Uint16 r = lhs.r();
  Uint16 g = lhs.g();
  Uint16 b = lhs.b();
  r = min(r + rhs.r(), 0xff);
  g = min(g + rhs.g(), 0xff);
  b = min(b + rhs.b(), 0xff);
  return Color(static_cast<Uint8>(r), static_cast<Uint8>(g),
               static_cast<Uint8>(b));
}

std::ostream &operator<<(std::ostream &lhs, const Color &rhs) {
  lhs << std::hex << static_cast<Uint32>(rhs) << std::dec;
  return lhs;
}
