#include "Client.h"

extern Renderer renderer;

void Client::initializeHelpWindow() {

    const px_t
        WIN_WIDTH = 400,
        WIN_HEIGHT = 250,
        WIN_X = (640 - WIN_WIDTH) / 2,
        WIN_Y = (360 - WIN_HEIGHT) / 2;

    _helpWindow = Window::WithRectAndTitle(
        Rect(WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT), "Help");

    
}
