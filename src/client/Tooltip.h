#ifndef TOOLTIP_BUILDER_H
#define TOOLTIP_BUILDER_H

#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>

#include "Texture.h"
#include "WordWrapper.h"
#include "../Color.h"
#include "../types.h"

class Tooltip{
    Color _color;
    std::vector<Texture> _content; // The lines of text; an empty texture implies a gap.

    static const px_t PADDING;
    static TTF_Font *font;
    const static px_t DEFAULT_MAX_WIDTH;

    static std::unique_ptr<WordWrapper> wordWrapper;

    Texture _generated{};
    void generate();

public:
    Tooltip();

    const static px_t NO_WRAP;

    void setFont(TTF_Font *font = nullptr); // Default: default font
    void setColor(const Color &color = Color::TOOLTIP_FONT);

    void addLine(const std::string &line);
    using Lines = std::vector<std::string>;
    void addLines(const Lines &lines);

    void addGap();

    const Texture &get();

    // Create a basic tooltip containing a single string.
    static Texture basicTooltip(const std::string &text);
};

#endif