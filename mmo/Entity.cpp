#include "Client.h"
#include "Entity.h"
#include "Renderer.h"

#include "util.h"

extern Renderer renderer;

Entity::Entity(const EntityType &type, const Point &location):
_type(type),
_location(location),
_yChanged(false),
_needsTooltipRefresh(false){}

SDL_Rect Entity::drawRect() const {
    return _type.drawRect() + _location;
}

void Entity::draw(const Client &client) const{
    _type.drawAt(_location + client.offset());
}

double Entity::bottomEdge() const{
    return _location.y + _type.drawRect().y + _type.height();
}

void Entity::location(const Point &loc){
    double oldY = _location.y;
    _location = loc;
    if (_location.y != oldY)
        _yChanged = true;
}

bool Entity::collision(const Point &p) const{
    return ::collision(p, drawRect());
}

void Entity::refreshTooltip(const Client &client){
    _needsTooltipRefresh = false;

    std::vector<std::string> textStrings = getTooltipMessages(client);
    if (textStrings.empty()) {
        _tooltip = Texture();
        return;
    }

    static const int PADDING = 10; // margins, and between title and body
    std::vector<Texture> textTextures;
    Texture heading(client.defaultFont(), textStrings.front(), Color::WHITE);
    textTextures.push_back(heading);
    int totalHeight = heading.height() + 2*PADDING;
    int totalWidth = heading.width() + 2*PADDING;

    if (textStrings.size() > 1) {
        totalHeight += PADDING;
        for (size_t i = 1; i < textStrings.size(); ++i) {
            static const Color textColor = Color::BLUE / 2 + Color::WHITE / 2;
            Texture nextMessage(client.defaultFont(), textStrings[i], textColor);
            textTextures.push_back(nextMessage);
            totalHeight += nextMessage.height();
            int tempWidth = nextMessage.width() + 2*PADDING;
            if (tempWidth > totalWidth)
                totalWidth = tempWidth;
        }

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
    std::vector<Texture>::const_iterator it = textTextures.begin();
    it->draw(PADDING, y);
    y += PADDING + it->height();
    for (++it; it != textTextures.end(); ++it) {
        it->draw(PADDING, y);
        y += it->height();
    }
    _tooltip.setBlend(SDL_BLENDMODE_BLEND);

    renderer.setRenderTarget();
}
