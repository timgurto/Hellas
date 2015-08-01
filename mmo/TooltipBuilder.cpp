#include "Renderer.h"
#include "TooltipBuilder.h"

extern Renderer renderer;

const int TooltipBuilder::PADDING = 4; // Margins, and the height of gaps between lines.
TTF_Font *TooltipBuilder::_defaultFont = 0;
const Color TooltipBuilder::DEFAULT_COLOR = Color::BLUE / 2 + Color::WHITE / 2;
const Color TooltipBuilder::BACKGROUND_COLOR = Color::WHITE/8 + Color::BLUE/6;

TooltipBuilder::TooltipBuilder():
_color(Color::WHITE)
{
    if (!_defaultFont)
        _defaultFont = TTF_OpenFont("trebuc.ttf", 10);
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
    Texture lineTexture(_font, line, _color);
    _content.push_back(lineTexture);
}

void TooltipBuilder::addGap(){
    _content.push_back(Texture());
}

void TooltipBuilder::publish(Texture &target){
    // Calculate height and width of final tooltip
    int
        totalHeight = 2*PADDING,
        totalWidth = 0;
    for (std::vector<Texture>::const_iterator it = _content.begin(); it != _content.end(); ++it){
        if (*it) {
            totalHeight += it->height();
            if (it->width() > totalWidth)
                totalWidth = it->width();
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
    target = Texture(totalWidth, totalHeight);
    target.setRenderTarget();
    background.draw();

    // Draw border
    renderer.setDrawColor(Color::WHITE);
    renderer.drawRect(makeRect(0, 0, totalWidth, totalHeight));

    // Draw text
    int y = PADDING;
    for (std::vector<Texture>::const_iterator it = _content.begin(); it != _content.end(); ++it){
        if (!*it)
            y += PADDING;
        else {
            it->draw(PADDING, y);
            y += it->height();
        }
    }

    target.setBlend(SDL_BLENDMODE_BLEND);
    renderer.setRenderTarget();
}