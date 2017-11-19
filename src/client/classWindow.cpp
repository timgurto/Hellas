#include "Client.h"

void Client::initializeClassWindow() {
    _classWindow = Window::WithRectAndTitle({ 80, 20, 500, 300 }, "Class"s);
}
