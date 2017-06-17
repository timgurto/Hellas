#include "ConfirmationWindow.h"
#include "Label.h"
#include "../Client.h"

ConfirmationWindow::ConfirmationWindow(const std::string &windowText, MessageCode msgCode,
                                       const std::string &msgArgs):
_msgCode(msgCode),
_msgArgs(msgArgs)
{
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    setPosition((Client::SCREEN_X - WINDOW_WIDTH) / 2, (Client::SCREEN_Y - WINDOW_HEIGHT) / 2); // TODO add a center() function
    setTitle("Confirmation");

    static const px_t
        PADDING = 2,
        BUTTON_WIDTH = 60,
        BUTTON_HEIGHT = 15,
        BUTTON_Y = 2 * PADDING + Element::TEXT_HEIGHT;

    addChild(new Label(Rect(0, PADDING, WINDOW_WIDTH, Element::TEXT_HEIGHT),
                       windowText, Element::CENTER_JUSTIFIED));
    px_t
        middle = WINDOW_WIDTH / 2,
        okButtonX = middle - PADDING/2 - BUTTON_WIDTH,
        cancelButtonX = middle + PADDING/2;
    addChild(new Button(Rect(okButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT),
                        "OK", sendMessageAndHideWindow, this));
    addChild(new Button(Rect(cancelButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT),
                        "Cancel", Window::hideWindow, this));

}

void ConfirmationWindow::sendMessageAndHideWindow(void *thisConfWindow){
    ConfirmationWindow *window = reinterpret_cast<ConfirmationWindow *>(thisConfWindow);
    Client::_instance->sendMessage(window->_msgCode, window->_msgArgs);
    window->hide();
}
