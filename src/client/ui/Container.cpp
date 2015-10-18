// (C) 2015 Tim Gurto

#include "ColorBlock.h"
#include "Container.h"
#include "../Client.h"
#include "../Renderer.h"
#include "../../server/User.h"

extern Renderer renderer;

const int Container::GAP = 0;

Container::Container(size_t rows, size_t cols, Item::vect_t &linked, int x, int y):
Element(Rect(x, y,
                 cols * (Client::ICON_SIZE + GAP + 2) + GAP,
                 rows * (Client::ICON_SIZE + GAP + 2) + GAP + 1)),
_rows(rows),
_cols(cols),
_linked(linked){
    for (size_t i = 0; i != Client::INVENTORY_SIZE; ++i) {
        const int
            x = i % cols,
            y = i / cols;
        static const Rect SLOT_BACKGROUND_OFFSET = Rect(1, 1, -2, -2);
        const Rect slotRect = Rect(x * (Client::ICON_SIZE + GAP + 2) + GAP,
                                           y * (Client::ICON_SIZE + GAP + 2) + GAP + 1,
                                           Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
        addChild(new ColorBlock(slotRect + SLOT_BACKGROUND_OFFSET, Color::BLACK));
        addChild(new ShadowBox(slotRect, true));
    }
}

void Container::refresh(){
    for (size_t i = 0; i != Client::INVENTORY_SIZE; ++i) {
        const int
            x = i % _cols,
            y = i / _cols;
        const std::pair<const Item *, size_t> &slot = _linked[i];
        if (slot.first){
            const Rect slotRect = Rect(x * (Client::ICON_SIZE + GAP + 2) + GAP,
                                               y * (Client::ICON_SIZE + GAP + 2) + GAP + 1,
                                               Client::ICON_SIZE + 2, Client::ICON_SIZE + 2);
            slot.first->icon().draw(slotRect.x + 1, slotRect.y + 1);
            if (slot.second > 1){
                Texture label(font(), makeArgs(slot.second), FONT_COLOR);
                label.draw(slotRect.x + slotRect.w - label.width() - 1,
                           slotRect.y + slotRect.h - label.height() + 1 + textOffset);
            }
        }
    }
}
