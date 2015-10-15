// (C) 2015 Tim Gurto

#include "Renderer.h"
#include "TooltipBuilder.h"

extern Renderer renderer;

bool TooltipBuilder::initialized = false;

const int TooltipBuilder::PADDING = 4; // Margins, and the height of gaps between lines.
TTF_Font *TooltipBuilder::_defaultFont = 0;
Color TooltipBuilder::DEFAULT_COLOR;
Color TooltipBuilder::BACKGROUND_COLOR;

TooltipBuilder::TooltipBuilder():
_color(Color::WHITE)
{
    if (!initialized) {
        DEFAULT_COLOR = Color::BLUE / 2 + Color::GREY_2;
        BACKGROUND_COLOR = Color::GREY_8 + Color::BLUE/6;
    }

    if (!_defaultFont)
        _defaultFont = TTF_OpenFont("04B_03__.TTF", 8);
    _font = _defaultFont;
}

void TooltipBuilder::setFont(TTF_Font *font){
    _font = font ? font : _defaultFont;
}

void TooltipBuilder::setColor(const Color &color){
    _color = color;
}

void TooltipBuilder::addLine(const std::string &line){
    if (line == "") {
        addGap();
        return;
    }
    const Texture lineTexture(_font, line, _color);
    _content.push_back(lineTexture);
}

void TooltipBuilder::addGap(){
    _content.push_back(Texture());
}

Texture TooltipBuilder::publish(){
    // Calculate height and width of final tooltip
    int
        totalHeight = 2*PADDING,
        totalWidth = 0;
    for (const Texture &item : _content){
        if (item) {
            totalHeight += item.height();
            if (item.width() > totalWidth)
                totalWidth = item.width();
        } else {
            totalHeight += PADDING;
        }
    }
    totalWidth += 2*PADDING;

    // Create background
    Texture background(totalWidth, totalHeight);
    background.setRenderTarget();
    renderer.setDrawColor(BACKGROUND_COLOR);
    renderer.clear();
    background.setAlpha(0xbf);

    // Draw background
    Texture ret = Texture(totalWidth, totalHeight);
    ret.setRenderTarget();
    background.draw();

    // Draw border
    renderer.setDrawColor(Color::WHITE);
    renderer.drawRect(makeRect(0, 0, totalWidth, totalHeight));

    // Draw text
    int y = PADDING;
    for (const Texture &item : _content){
        if (!item)
            y += PADDING;
        else {
            item.draw(PADDING, y);
            y += item.height();
        }
    }

    ret.setBlend(SDL_BLENDMODE_BLEND);
    renderer.setRenderTarget();
    return ret;
}