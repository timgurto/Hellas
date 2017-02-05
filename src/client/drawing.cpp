#include "Client.h"
#include "ClientNPC.h"
#include "Renderer.h"
#include "ui/Container.h"

extern Renderer renderer;

void Client::draw() const{
    if (!_loggedIn || !_loaded){
        renderer.setDrawColor();
        renderer.clear();
        _chatLog->draw();
        renderer.present();
        return;
    }

    // Background
    renderer.setDrawColor(); // TODO: Revert to BLUE_HELL
    renderer.clear();

    // Map
    size_t
        xMin = static_cast<size_t>(max<double>(0, -offset().x / TILE_W)),
        xMax = static_cast<size_t>(min<double>(_mapX,
                                               1.0 * (-offset().x + SCREEN_X) / TILE_W + 1.5)),
        yMin = static_cast<size_t>(max<double>(0, -offset().y / TILE_H)),
        yMax = static_cast<size_t>(min<double>(_mapY, (-offset().y + SCREEN_Y) / TILE_H + 1));
    for (size_t y = yMin; y != yMax; ++y) {
        const px_t yLoc = y * TILE_H + toInt(offset().y);
        for (size_t x = xMin; x != xMax; ++x){
            px_t xLoc = x * TILE_W + toInt(offset().x);
            if (y % 2 == 1)
                xLoc -= TILE_W/2;
            drawTile(x, y, xLoc, yLoc);
        }
    }

    // Base under target NPC
    if (_targetNPC != nullptr){
        const Texture &base = aggressive() ? _baseAggressive : _basePassive;
        static const Point BASE_OFFSET(-15, -10);
        base.draw(targetNPC()->location() + offset() + BASE_OFFSET);
    }

    // Character's target and actual location
    if (isDebug()) {
        renderer.setDrawColor(Color::CYAN);
        const Point &actualLoc = _character.destination() + offset();
        renderer.drawRect(Rect(actualLoc.x - 1, actualLoc.y - 1, 3, 3));

        renderer.setDrawColor(Color::WHITE);
        Point pendingLoc(_pendingCharLoc.x + offset().x, _pendingCharLoc.y + offset().y);
        renderer.drawRect(Rect(pendingLoc.x, pendingLoc.y, 1, 1));
        renderer.drawRect(Rect(pendingLoc.x - 2, pendingLoc.y - 2, 5, 5));
    }

    // Entities, sorted from back to front
    static const px_t
        DRAW_MARGIN_ABOVE = 50,
        DRAW_MARGIN_BELOW = 10,
        DRAW_MARGIN_SIDES = 30;
    const double
        topY = -offset().y - DRAW_MARGIN_BELOW,
        bottomY = -offset().y + SCREEN_Y + DRAW_MARGIN_ABOVE,
        leftX = -offset().x - DRAW_MARGIN_SIDES,
        rightX = -offset().x + SCREEN_X + DRAW_MARGIN_SIDES;
    // Cull by y
    Entity
        topEntity(nullptr, Point(0, topY)),
        bottomEntity(nullptr, Point(0, bottomY));
    auto top = _entities.lower_bound(&topEntity);
    auto bottom = _entities.upper_bound(&bottomEntity);
    // Flat entities
    for (auto it = top; it != bottom; ++it) {
        if ((*it)->isFlat()) {
            // Cull by x
            double x = (*it)->location().x;
            if (x >= leftX && x <= rightX)
                (*it)->draw(*this);
        }
    }
    // Non-flat entities
    for (auto it = top; it != bottom; ++it) {
        if (!(*it)->isFlat()) {
            // Cull by x
            double x = (*it)->location().x;
            if (x >= leftX && x <= rightX)
                (*it)->draw(*this);
        }
    }

    // Non-window UI
    for (Element *element : _ui)
        element->draw();

    // Windows
    for (windows_t::const_reverse_iterator it = _windows.rbegin(); it != _windows.rend(); ++it)
        (*it)->draw();

    // Dragged item
    static const Point MOUSE_ICON_OFFSET(-Client::ICON_SIZE/2, -Client::ICON_SIZE/2);
    const ClientItem *draggedItem = Container::getDragItem();
    if (draggedItem != nullptr)
        draggedItem->icon().draw(_mouse + MOUSE_ICON_OFFSET);

    // Used item
    if (_constructionFootprint) {
        const ClientObjectType *ot = Container::getUseItem()->constructsObject();
        Rect footprintRect = ot->collisionRect() + _mouse - _offset;
        if (distance(playerCollisionRect(), footprintRect) <= Client::ACTION_DISTANCE) {
            renderer.setDrawColor(Color::FOOTPRINT_GOOD);
            renderer.fillRect(footprintRect + _offset);

            const Rect &drawRect = ot->drawRect();
            px_t
                x = toInt(_mouse.x + drawRect.x),
                y = toInt(_mouse.y + drawRect.y);
            _constructionFootprint.setAlpha(0x7f);
            _constructionFootprint.draw(x, y);
            _constructionFootprint.setAlpha();
        } else {
            renderer.setDrawColor(Color::FOOTPRINT_BAD);
            renderer.fillRect(footprintRect + _offset);
        }
    } else if (_isDismounting){
        const EntityType &charType = *_character.type();
        Rect footprintRect = Avatar::collisionRectRaw() + _mouse - _offset;
        if (distance(playerCollisionRect(), footprintRect) <= Client::ACTION_DISTANCE) {
            charType.image().setAlpha(0x7f);
            charType.image().draw(_mouse + charType.drawRect());
            charType.image().setAlpha();
        } else {
            renderer.setDrawColor(Color::FOOTPRINT_BAD);
            renderer.fillRect(footprintRect + _offset);
        }
    }

    // Cull distance
    if (isDebug()){
        Point midScreen = _character.location() + offset();
        renderer.setDrawColor(Color::RED);
        renderer.drawRect(Rect(midScreen.x - CULL_DISTANCE,
                               midScreen.y - CULL_DISTANCE,
                               CULL_DISTANCE * 2,
                               CULL_DISTANCE * 2));
    }

    // Tooltip
    drawTooltip();

    // Cursor
    _currentCursor->draw(_mouse);

    renderer.present();
    _drawingFinished = true;
}

void Client::drawTooltip() const{
    const Texture *tooltip;
    if (Element::tooltip() != nullptr)
        tooltip = Element::tooltip();
    else if (_currentMouseOverEntity != nullptr && !_mouseOverWindow)
        tooltip = &_currentMouseOverEntity->tooltip();
    else
        return;

    if (tooltip != nullptr) {
        static const px_t EDGE_GAP = 2; // Gap from screen edges
        static const px_t CURSOR_GAP = 10; // Horizontal gap from cursor
        px_t x, y;
        const px_t mouseX = toInt(_mouse.x);
        const px_t mouseY = toInt(_mouse.y);

        // y: below cursor, unless too close to the bottom of the screen
        if (SCREEN_Y > mouseY + tooltip->height() + EDGE_GAP)
            y = mouseY;
        else
            y = SCREEN_Y - tooltip->height() - EDGE_GAP;

        // x: to the right of the cursor, unless too close to the right of the screen
        if (SCREEN_X > mouseX + tooltip->width() + EDGE_GAP + CURSOR_GAP)
            x = mouseX + CURSOR_GAP;
        else
            x = mouseX - tooltip->width() - CURSOR_GAP;
        tooltip->draw(x, y);
    }
}

void Client::drawTile(size_t x, size_t y, px_t xLoc, px_t yLoc) const{
    if (isDebug()) {
        _terrain.at(_map[x][y]).draw(xLoc, yLoc);
        return;
    }


    /*
          H | E
      L | tileID| R
          G | F
    */
    const Rect drawLoc(xLoc, yLoc, 0, 0);
    const bool yOdd = (y % 2 == 1);
    size_t tileID, L, R, E, F, G, H;
    tileID = _map[x][y];
    R = x == _mapX-1 ? tileID : _map[x+1][y];
    L = x == 0 ? tileID : _map[x-1][y];
    if (y == 0) {
        H = E = tileID;
    } else {
        if (yOdd) {
            E = _map[x][y-1];
            H = x == 0 ? tileID : _map[x-1][y-1];
        } else {
            E = x == _mapX-1 ? tileID : _map[x+1][y-1];
            H = _map[x][y-1];
        }
    }
    if (y == _mapY-1) {
        G = F = tileID;
    } else {
        if (!yOdd) {
            F = x == _mapX-1 ? tileID : _map[x+1][y+1];
            G = _map[x][y+1];
        } else {
            F = _map[x][y+1];
            G = x == 0 ? tileID : _map[x-1][y+1];
        }
    }

    static const Rect
        TOP_LEFT     (0,        0,        TILE_W/2, TILE_H/2),
        TOP_RIGHT    (TILE_W/2, 0,        TILE_W/2, TILE_H/2),
        BOTTOM_LEFT  (0,        TILE_H/2, TILE_W/2, TILE_H/2),
        BOTTOM_RIGHT (TILE_W/2, TILE_H/2, TILE_W/2, TILE_H/2),
        LEFT_HALF    (0,        0,        TILE_W/2, TILE_H),
        RIGHT_HALF   (TILE_W/2, 0,        TILE_W/2, TILE_H),
        FULL         (0,        0,        TILE_W,   TILE_H);

    // Black background
    // Assuming all tile images are set to SDL_BLENDMODE_ADD and quarter alpha
    renderer.setDrawColor(Color::BLACK);
    if (yOdd && x == 0) {
        renderer.fillRect(drawLoc + RIGHT_HALF);
    }
    else if (!yOdd && x == _mapX-1) {
        renderer.fillRect(drawLoc + LEFT_HALF);
    }
    else {
        renderer.fillRect(drawLoc + FULL);
    }

    // Half-alpha base tile
    _terrain.at(tileID).setHalfAlpha();
    if (yOdd && x == 0) {
        _terrain.at(tileID).draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain.at(tileID).draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    } else if (!yOdd && x == _mapX-1) {
        _terrain.at(tileID).draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain.at(tileID).draw(drawLoc + TOP_LEFT, TOP_LEFT);
    } else {
        _terrain.at(tileID).draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain.at(tileID).draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
        _terrain.at(tileID).draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain.at(tileID).draw(drawLoc + TOP_LEFT, TOP_LEFT);
    }
    _terrain.at(tileID).setQuarterAlpha();

    // Quarter-alpha L, R, E, F, G, H tiles
    if (!yOdd || x != 0) {
        _terrain.at(L).draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain.at(L).draw(drawLoc + TOP_LEFT, TOP_LEFT);
        _terrain.at(G).draw(drawLoc + BOTTOM_LEFT, BOTTOM_LEFT);
        _terrain.at(H).draw(drawLoc + TOP_LEFT, TOP_LEFT);
    }
    if (yOdd || x != _mapX-1) {
        _terrain.at(R).draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain.at(R).draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
        _terrain.at(E).draw(drawLoc + TOP_RIGHT, TOP_RIGHT);
        _terrain.at(F).draw(drawLoc + BOTTOM_RIGHT, BOTTOM_RIGHT);
    }

    /*if (!_terrain[tileID].isTraversable()) {
        renderer.setDrawColor(Color::RED);
        renderer.drawRect(drawLoc + FULL);
    }*/
}
