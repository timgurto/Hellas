#include "Branch.h"
#include "Client.h"
#include "Color.h"
#include "Renderer.h"
#include "Server.h"
#include "util.h"

extern Renderer renderer;

EntityType Branch::_entityType(makeRect(-10, -5));

Branch::Branch(const Branch &rhs):
Entity(rhs),
_serial(rhs._serial){}

Branch::Branch(size_t serialArg, const Point &loc):
Entity(_entityType, loc),
_serial(serialArg){}

void Branch::onLeftClick(const Client &client) const{
    std::ostringstream oss;
    oss << '[' << CL_COLLECT_BRANCH << ',' << _serial << ']';
    client.socket().sendMessage(oss.str());
}

void Branch::refreshTooltip(const Client &client){
    static const int PADDING = 10;
    Texture title(client.defaultFont(), "Branch", Color::WHITE);
    int totalHeight = title.height() + 2*PADDING;
    int totalWidth = title.width() + 2*PADDING;

    Texture extra;
    if (distance(location(), client.character().location()) > Server::ACTION_DISTANCE) {
        static const Color textColor = Color::BLUE / 2 + Color::WHITE / 2;
        extra = Texture(client.defaultFont(), "Out of range", textColor);
        totalHeight += extra.height() + PADDING;
        int tempWidth = extra.width() + 2*PADDING;
        if (tempWidth > totalWidth)
            totalWidth = tempWidth;
    }

    _tooltip = Texture(totalWidth, totalHeight);
    
    // Draw background
    Texture background(totalWidth, totalHeight);
    static const Color backgroundColor = Color::WHITE/8 + Color::BLUE/6;
    background.setRenderTarget();
    renderer.setDrawColor(backgroundColor);
    renderer.clear();
    background.setBlend(SDL_BLENDMODE_NONE, 0xbf);

    _tooltip.setRenderTarget();
    background.draw();
    renderer.setDrawColor(Color::WHITE);
    renderer.drawRect(makeRect(0, 0, totalWidth-1, totalHeight-1));

    // Draw text
    int y = PADDING;
    title.draw(PADDING, y);
    if (extra) {
        y += title.height() + PADDING;
        extra.draw(PADDING, y);
    }
    _tooltip.setBlend(SDL_BLENDMODE_BLEND);

    renderer.setRenderTarget();
}
