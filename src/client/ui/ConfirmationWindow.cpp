#include "ConfirmationWindow.h"
#include "Label.h"
#include "TextBox.h"
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

InfoWindow::InfoWindow(const std::string & windowText) {
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    setPosition((Client::SCREEN_X - WINDOW_WIDTH) / 2, (Client::SCREEN_Y - WINDOW_HEIGHT) / 2); // TODO add a center() function
    setTitle("Info");

    static const px_t
        PADDING = 2,
        BUTTON_WIDTH = 60,
        BUTTON_HEIGHT = 15,
        BUTTON_Y = 2 * PADDING + Element::TEXT_HEIGHT;

    addChild(new Label(Rect(0, PADDING, WINDOW_WIDTH, Element::TEXT_HEIGHT),
        windowText, Element::CENTER_JUSTIFIED));
    px_t
        middle = WINDOW_WIDTH / 2,
        okButtonX = middle - BUTTON_WIDTH / 2;
    addChild(new Button(Rect(okButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT),
        "OK", hideWindow, this));
}

void InfoWindow::deleteWindow(void *thisWindow) {
    auto *window = reinterpret_cast<Window *>(thisWindow);
    Client::_instance->removeWindow(window);
}

InputWindow::InputWindow(const std::string & windowText, MessageCode msgCode, const std::string & msgArgs):
_msgCode(msgCode),
_msgArgs(msgArgs)
{
    const px_t
        WINDOW_WIDTH = 300,
        WINDOW_HEIGHT = 45,
        PADDING = 2,
        BUTTON_WIDTH = 60,
        BUTTON_HEIGHT = 15;

    resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    setPosition((Client::SCREEN_X - WINDOW_WIDTH) / 2, (Client::SCREEN_Y - WINDOW_HEIGHT) / 2); // TODO add a center() function
    setTitle("Input");


    auto y = PADDING;
    addChild(new Label(Rect(0, y, WINDOW_WIDTH, Element::TEXT_HEIGHT),
        windowText, Element::CENTER_JUSTIFIED));
    y += Element::TEXT_HEIGHT + PADDING;

    _textBox = new TextBox({ 0, y, WINDOW_WIDTH, Element::TEXT_HEIGHT });
    addChild(_textBox);
    y += _textBox->height() + PADDING;

    px_t
        middle = WINDOW_WIDTH / 2,
        okButtonX = middle - PADDING / 2 - BUTTON_WIDTH,
        cancelButtonX = middle + PADDING / 2;
    addChild(new Button(Rect(okButtonX, y, BUTTON_WIDTH, BUTTON_HEIGHT),
        "OK", sendMessageWithInputAndHideWindow, this));
    addChild(new Button(Rect(cancelButtonX, y, BUTTON_WIDTH, BUTTON_HEIGHT),
        "Cancel", Window::hideWindow, this));
}

void InputWindow::sendMessageWithInputAndHideWindow(void * thisInputWindow) {
    auto *window = reinterpret_cast<InputWindow *>(thisInputWindow);
    auto args = makeArgs(window->_msgArgs, window->_textBox->text());
    Client::_instance->sendMessage(window->_msgCode, args);
    window->hide();
}
