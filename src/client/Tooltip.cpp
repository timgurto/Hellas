#include <sstream>

#include "Renderer.h"
#include "Tooltip.h"

extern Renderer renderer;

const px_t Tooltip::PADDING = 4; // Margins, and the height of gaps between lines.
TTF_Font *Tooltip::font = nullptr;
const px_t Tooltip::DEFAULT_MAX_WIDTH = 150;
const px_t Tooltip::NO_WRAP = 0;

Tooltip::Tooltip()
{
    _color = Color::TOOLTIP_FONT;

    if (font == nullptr)
        font = TTF_OpenFont("AdvoCut.ttf", 10);
    font = font;
}

void Tooltip::setFont(TTF_Font *font){
    font = font ? font : font;
}

void Tooltip::setColor(const Color &color){
    _color = color;
}

void Tooltip::addLine(const std::string &line){
    if (line == "") {
        addGap();
        return;
    }
    const Texture lineTexture(font, line, _color);
    _content.push_back(lineTexture);
}

void Tooltip::addLines(const Lines & lines) {
    for (auto &line : lines)
        addLine(line);
}

void Tooltip::addGap(){
    _content.push_back(Texture());
}

Texture Tooltip::publish(){
    // Calculate height and width of final tooltip
    px_t
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
    renderer.pushRenderTarget(background);
    renderer.setDrawColor(Color::TOOLTIP_BACKGROUND);
    renderer.clear();
    background.setAlpha(0xdf);
    renderer.popRenderTarget();

    // Draw background
    Texture ret = Texture(totalWidth, totalHeight);
    renderer.pushRenderTarget(ret);
    background.draw();

    // Draw border
    renderer.setDrawColor(Color::TOOLTIP_BORDER);
    renderer.drawRect({ 0, 0, totalWidth, totalHeight });

    // Draw text
    px_t y = PADDING;
    for (const Texture &item : _content){
        if (!item)
            y += PADDING;
        else {
            item.draw(PADDING, y);
            y += item.height();
        }
    }

    ret.setBlend(SDL_BLENDMODE_BLEND);
    renderer.popRenderTarget();
    return ret;
}

Texture Tooltip::basicTooltip(const std::string &text, px_t maxWidth)
{
    Tooltip tb;
    
    { // Try a single line
        Texture lineTexture(font, text, tb._color);
        if (maxWidth == NO_WRAP || lineTexture.width() <= maxWidth) { // No wrapping necessary.
            tb._content.push_back(lineTexture);
            return tb.publish();
        }
    }

    std::istringstream iss(text);
    static const size_t BUFFER_SIZE = 50; // Maximum word length
    static char buffer[BUFFER_SIZE];

    std::string segment;
    std::string extraSpaces;
    while (!iss.eof()) {
        iss.get(buffer, BUFFER_SIZE, ' '); iss.ignore(1);
        std::string word(buffer);
        word = extraSpaces + word;
        extraSpaces = "";
        while (iss.peek() == ' ') {
            extraSpaces += " ";
            iss.ignore(1);
        }
        Texture lineTexture(font, segment + " " +  word, tb._color);
        if (lineTexture.width() > maxWidth) {
            if (segment == "") {
                tb._content.push_back(lineTexture);
                continue;
            } else {
                tb.addLine(segment);
                segment = word;
                continue;
            }
        }
        if (segment != "")
            segment += " ";
        segment += word;
    }
    tb.addLine(segment);
    
    return tb.publish();
}
