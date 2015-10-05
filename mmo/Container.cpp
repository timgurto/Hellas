// (C) 2015 Tim Gurto

#include "ColorBlock.h"
#include "Container.h"
#include "Renderer.h"
#include "User.h"

extern Renderer renderer;

const int Container::GAP = 0;

Container::Container(size_t rows, size_t cols, containerVec_t &linked, int x, int y):
Element(makeRect(x, y,
                 cols * (Client::ICON_SIZE + GAP) + GAP,
                 rows * (Client::ICON_SIZE + GAP) + GAP + 1)),
_rows(rows),
_cols(cols),
_linked(linked){
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const int
            x = i % cols,
            y = i / cols;
        static const SDL_Rect SLOT_BACKGROUND_OFFSET = makeRect(1, 1, -2, -2);
        const SDL_Rect slotRect = makeRect(x * (Client::ICON_SIZE + GAP) + GAP,
                                           y * (Client::ICON_SIZE + GAP) + GAP + 1,
                                           Client::ICON_SIZE, Client::ICON_SIZE);
        addChild(new ColorBlock(slotRect + SLOT_BACKGROUND_OFFSET, Color::BLACK));
        addChild(new ShadowBox(slotRect, true));
    }
}
