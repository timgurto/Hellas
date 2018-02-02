#include <sstream>

#include "Client.h"
#include "Renderer.h"
#include "Tooltip.h"

extern Renderer renderer;

const px_t Tooltip::PADDING = 4; // Margins, and the height of gaps between lines.
TTF_Font *Tooltip::font = nullptr;
const px_t Tooltip::DEFAULT_MAX_WIDTH = 150;
const px_t Tooltip::NO_WRAP = 0;
std::unique_ptr<WordWrapper> Tooltip::wordWrapper;
ms_t Tooltip::timeThatTheLastRedrawWasOrdered{};
const Tooltip Tooltip::NO_TOOLTIP{};

Tooltip::Tooltip()
{
    _color = Color::TOOLTIP_FONT;

    if (font == nullptr)
        font = TTF_OpenFont("AdvoCut.ttf", 10);
}

void Tooltip::setColor(const Color &color){
    _color = color;
}

void Tooltip::addLine(const std::string &line){
    if (line == "") {
        addGap();
        return;
    }

    if (!wordWrapper) {
        wordWrapper = std::make_unique<WordWrapper>(WordWrapper(font, DEFAULT_MAX_WIDTH));
    }
    auto wrappedLines = wordWrapper->wrap(line);
    for (const auto &wrappedLine : wrappedLines)
        _content.push_back({ font, wrappedLine, _color });
}

void Tooltip::addLines(const Lines & lines) {
    for (auto &line : lines)
        addLine(line);
}

void Tooltip::embed(const Tooltip & subTooltip) {
    subTooltip.generateIfNecessary();
    _content.push_back(subTooltip._generated);
}

px_t Tooltip::width() const {
    generateIfNecessary();
    return _generated.width();
}

px_t Tooltip::height() const {
    generateIfNecessary();
    return _generated.height();
}

void Tooltip::addGap(){
    _content.push_back(Texture());
}

void Tooltip::draw(ScreenPoint p) const {
    const auto &client = Client::instance();
    generateIfNecessary();
    if (this != &NO_TOOLTIP)
        _generated.draw(p.x, p.y);
}

void Tooltip::forceAllToRedraw() {
    timeThatTheLastRedrawWasOrdered = SDL_GetTicks();
}

void Tooltip::generateIfNecessary() const {
    if (!_generated || _timeGenerated < timeThatTheLastRedrawWasOrdered)
        generate();
}

void Tooltip::generate() const {
    // Calculate height and width of final tooltip
    px_t
        totalHeight = 2 * PADDING,
        totalWidth = 0;
    for (const Texture &item : _content) {
        if (item) {
            totalHeight += item.height();
            if (item.width() > totalWidth)
                totalWidth = item.width();
        } else {
            totalHeight += PADDING;
        }
    }
    totalWidth += 2 * PADDING;

    // Create background
    Texture background(totalWidth, totalHeight);
    renderer.pushRenderTarget(background);
    renderer.setDrawColor(Color::TOOLTIP_BACKGROUND);
    renderer.clear();
    background.setAlpha(0xdf);
    renderer.popRenderTarget();

    // Draw background
    _generated = Texture{ totalWidth, totalHeight };
    renderer.pushRenderTarget(_generated);
    background.draw();

    // Draw border
    renderer.setDrawColor(Color::TOOLTIP_BORDER);
    renderer.drawRect({ 0, 0, totalWidth, totalHeight });

    // Draw text
    px_t y = PADDING;
    for (const Texture &item : _content) {
        if (!item)
            y += PADDING;
        else {
            item.draw(PADDING, y);
            y += item.height();
        }
    }

    _generated.setBlend(SDL_BLENDMODE_BLEND);
    renderer.popRenderTarget();

    _timeGenerated = SDL_GetTicks();
}

Tooltip Tooltip::basicTooltip(const std::string &text)
{
    Tooltip tb;
    tb.addLine(text);
    return tb;
}
