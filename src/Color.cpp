#include "Color.h"
#include "util.h"

const Color Color::BLACK(0x00, 0x00, 0x00);
const Color Color::BLUE(0x00, 0x00, 0xff);
const Color Color::GREEN(0x00, 0xff, 0x00);
const Color Color::CYAN(0x00, 0xff, 0xff);
const Color Color::RED(0xff, 0x00, 0x00);
const Color Color::MAGENTA(0xff, 0x00, 0xff);
const Color Color::YELLOW(0xff, 0xff, 0x00);
const Color Color::WHITE(0xff, 0xff, 0xff);

// clang-format off
Color
  Color::WARNING,
  Color::FAILURE,
  Color::SUCCESS,

  Color::CHAT_LOG_BACKGROUND,
  Color::SAY,
  Color::WHISPER,

  Color::DEFAULT_DRAW,
  Color::FONT,
  Color::FONT_OUTLINE,
  Color::DISABLED_TEXT,

  Color::TOOLTIP_FONT,
  Color::TOOLTIP_BACKGROUND,
  Color::TOOLTIP_BORDER,
  Color::FLAVOUR_TEXT,

  Color::ELEMENT_BACKGROUND,
  Color::ELEMENT_SHADOW_DARK,
  Color::ELEMENT_SHADOW_LIGHT,
  Color::ELEMENT_FONT,
  Color::CONTAINER_SLOT_BACKGROUND,

  Color::ITEM_NAME,
  Color::ITEM_STATS,
  Color::ITEM_INSTRUCTIONS, 
  Color::ITEM_TAGS,

  Color::HELP_TEXT_HEADING,

  Color::QUEST_NAME,
  Color::QUEST_OBJECTIVE,

  Color::FOOTPRINT_GOOD,
  Color::FOOTPRINT_BAD,
  Color::FOOTPRINT,
  Color::IN_RANGE,
  Color::OUT_OF_RANGE,
  Color::HEALTH_BAR_BACKGROUND,
  Color::HEALTH_BAR_OUTLINE,
  Color::CAST_BAR_FONT,
  Color::PERFORMANCE_FONT,
  Color::PROGRESS_BAR,
  Color::PROGRESS_BAR_BACKGROUND,
  Color::COMBATANT_SELF,
  Color::COMBATANT_ALLY,
  Color::COMBATANT_NEUTRAL,
  Color::COMBATANT_ENEMY,
  Color::ENERGY,
  Color::PLAYER_NAME_OUTLINE,
  Color::OUTLINE,
  Color::HIGHLIGHT_OUTLINE,

  Color::FLOATING_HEAL,
  Color::FLOATING_DAMAGE,
  Color::FLOATING_XP,
  Color::FLOATING_MISS,
  Color::FLOATING_LOOT,
    
  Color::AIR,
  Color::EARTH,
  Color::FIRE,
  Color::WATER;
// clang-format on

const Color Color::GREY_2(0x80, 0x80, 0x80);
const Color Color::GREY_4(0x40, 0x40, 0x40);
const Color Color::GREY_8(0x20, 0x20, 0x20);

// const Color Color::BLUE_HELL(0x18, 0x52, 0xa1);
const Color Color::NO_KEY(0x01, 0x02, 0x03);

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
