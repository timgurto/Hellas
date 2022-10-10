#ifndef COLOR_H
#define COLOR_H

#include <iostream>

#include "SDL.h"

class Color {
 public:
  static const Color BLUE_HELL;
  static const Color NO_KEY;
  static const Color DEBUG_TEXT;
  static const Color MISSING_IMAGE;

  static const Color BLACK;
  static const Color BLUE;
  static const Color GREEN;
  static const Color CYAN;
  static const Color RED;
  static const Color MAGENTA;
  static const Color YELLOW;
  static const Color WHITE;

  static const Color CHAT_BACKGROUND;
  static const Color CHAT_DEFAULT;
  static const Color CHAT_SAY;
  static const Color CHAT_WHISPER;
  static const Color CHAT_WARNING;
  static const Color CHAT_ERROR;
  static const Color CHAT_SUCCESS;

  static const Color WINDOW_BACKGROUND;
  static const Color WINDOW_DARK;
  static const Color WINDOW_LIGHT;
  static const Color WINDOW_FONT;
  static const Color WINDOW_FLAVOUR_TEXT;
  static const Color WINDOW_HEADING;

  static const Color COMBATANT_SELF;
  static const Color COMBATANT_ALLY;
  static const Color COMBATANT_DEFENSIVE;
  static const Color COMBATANT_ENEMY;
  static const Color COMBATANT_NEUTRAL;

  static const Color FOOTPRINT_GOOD;
  static const Color FOOTPRINT_BAD;
  static const Color FOOTPRINT_COLLISION;

  static const Color TOOLTIP_BACKGROUND;
  static const Color TOOLTIP_BORDER;
  static const Color TOOLTIP_NAME;
  static const Color TOOLTIP_BODY;
  static const Color TOOLTIP_TAG;
  static const Color TOOLTIP_INSTRUCTION;
  static const Color TOOLTIP_FLAVOUR;
  static const Color TOOLTIP_BAD;

  static const Color TOOL_PRESENT;
  static const Color TOOL_MISSING;

  static const Color ITEM_QUALITY_COMMON;
  static const Color ITEM_QUALITY_UNCOMMON;
  static const Color ITEM_QUALITY_RARE;
  static const Color ITEM_QUALITY_EPIC;
  static const Color ITEM_QUALITY_LEGENDARY;

  static const Color STAT_HEALTH;
  static const Color STAT_ENERGY;
  static const Color STAT_XP;
  static const Color STAT_XP_BONUS;
  static const Color STAT_AIR;
  static const Color STAT_EARTH;
  static const Color STAT_FIRE;
  static const Color STAT_WATER;

  static const Color FLOATING_CORE;
  static const Color FLOATING_DAMAGE;
  static const Color FLOATING_HEAL;
  static const Color FLOATING_MISS;
  static const Color FLOATING_CRIT;
  static const Color FLOATING_INFO;

  static const Color BUFF;
  static const Color DEBUFF;

  static const Color UI_OUTLINE;
  static const Color UI_OUTLINE_HIGHLIGHT;
  static const Color UI_FEEDBACK;  // Instructions; server messages
  static const Color UI_TEXT;
  static const Color UI_DISABLED;
  static const Color UI_PROGRESS_BAR;

  static const Color RELEASE_NOTES_VERSION;
  static const Color RELEASE_NOTES_SUBHEADING;
  static const Color RELEASE_NOTES_BODY;

  static const Color CHANCE_SMALL;
  static const Color CHANCE_MODERATE;
  static const Color CHANCE_HIGH;

  static const Color SPRITE_OUTLINE;
  static const Color SPRITE_OUTLINE_HIGHLIGHT;

  static const Color DURABILITY_LOW;
  static const Color DURABILITY_BROKEN;

  static const Color DIFFICULTY_VERY_HIGH;
  static const Color DIFFICULTY_HIGH;
  static const Color DIFFICULTY_NEUTRAL;
  static const Color DIFFICULTY_LOW;
  static const Color DIFFICULTY_VERY_LOW;

  Color(Uint8 r, Uint8 g, Uint8 b);
  Color(const SDL_Color &rhs);
  Color(Uint32 rhs = 0);

  operator SDL_Color() const;
  operator Uint32() const;  // Used by some SDL functions

  // Multiply or divide color by a scalar
  Color operator/(double s) const;
  Color operator/(int s) const;  // to avoid ambiguity with implicit Uint32 cast
  Color operator*(double s) const;
  Color operator*(int s) const;  // to avoid ambiguity with implicit Uint32 cast

  Uint8 r() const { return _r; }
  Uint8 g() const { return _g; }
  Uint8 b() const { return _b; }

 private:
  Uint8 _r, _g, _b;
};

Color operator+(const Color &lhs, const Color &rhs);

std::ostream &operator<<(std::ostream &lhs, const Color &rhs);

#endif
