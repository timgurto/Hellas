#include "Client.h"
#include "OtherUser.h"
#include "Renderer.h"
#include "util.h"

extern Renderer renderer;

EntityType OtherUser::_entityType(makeRect(-9, -39));

OtherUser::OtherUser(const std::string &name, const Point &location):
Entity(_entityType, location),
_name(name){}

Point OtherUser::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void OtherUser::draw(const Client &client) const{
    Entity::draw(client);

    // Draw username
    Texture nameTexture(client.defaultFont(), _name, Color::CYAN);
    Point p = location();
    p.y -= 60;
    p.x -= nameTexture.width() / 2;
    nameTexture.draw(p);
}

void OtherUser::update(double delta){
    location(interpolatedLocation(delta));
}

void OtherUser::refreshTooltip(const Client &client){
    static const int PADDING = 10;
    Texture title(client.defaultFont(), _name, Color::WHITE);
    int totalHeight = title.height() + 2*PADDING;
    int totalWidth = title.width() + 2*PADDING;

    Texture extra;
    static const Color textColor = Color::BLUE / 2 + Color::WHITE / 2;
    extra = Texture(client.defaultFont(), "Player", textColor);
    totalHeight += extra.height() + PADDING;
    int tempWidth = extra.width() + 2*PADDING;
    if (tempWidth > totalWidth)
        totalWidth = tempWidth;

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
