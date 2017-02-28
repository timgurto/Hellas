#include <cassert>

#include "Client.h"

extern Renderer renderer;

void Client::initializeBuildWindow(){
    _buildWindow = new Window(Rect(100, 50, 50, 150), "Construction");

    /*

    static const px_t
        FILTERS_PANE_W = 150,
        RECIPES_PANE_W = 160,
        DETAILS_PANE_W = 150,
        PANE_GAP = 6,
        FILTERS_PANE_X = PANE_GAP / 2,
        RECIPES_PANE_X = FILTERS_PANE_X + FILTERS_PANE_W + PANE_GAP,
        DETAILS_PANE_X = RECIPES_PANE_X + RECIPES_PANE_W + PANE_GAP,
        CRAFTING_WINDOW_W = DETAILS_PANE_X + DETAILS_PANE_W + PANE_GAP/2,

        CONTENT_H = 200,
        CONTENT_Y = PANE_GAP/2,
        CRAFTING_WINDOW_H = CONTENT_Y + CONTENT_H + PANE_GAP/2;

    _craftingWindow = new Window(Rect(100, 50, CRAFTING_WINDOW_W, CRAFTING_WINDOW_H), "Crafting");
    _craftingWindow->addChild(new Line(RECIPES_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));
    _craftingWindow->addChild(new Line(DETAILS_PANE_X - PANE_GAP/2, CONTENT_Y,
                                       CONTENT_H, Element::VERTICAL));*/
}
