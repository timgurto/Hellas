#include "Client.h"

extern Renderer renderer;

void Client::initializeSocialWindow() {

    const px_t
        GAP = 2,
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = Element::TEXT_HEIGHT,
        WIN_WIDTH = BUTTON_WIDTH + 2 * GAP,
        WIN_HEIGHT = BUTTON_HEIGHT + 2 * GAP;

    _socialWindow = Window::WithRectAndTitle(
        Rect(WIN_WIDTH, WIN_HEIGHT, BUTTON_WIDTH, WIN_HEIGHT), "Social");

    static auto LEAVE_CITY_MESSAGE = compileMessage(CL_LEAVE_CITY);
    _socialWindow->addChild(new Button({ GAP, GAP, BUTTON_WIDTH, BUTTON_HEIGHT }, "Leave city",
            sendRawMessageStatic, &LEAVE_CITY_MESSAGE));
}
