#ifndef CONFIRMATION_WINDOW_H
#define CONFIRMATION_WINDOW_H

#include "Element.h"
#include "Window.h"

class ConfirmationWindow : public Window {
public:
    ConfirmationWindow(const std::string &windowText, MessageCode msgCode,
        const std::string &msgArgs);

private:
    MessageCode _msgCode;
    std::string _msgArgs;

    static const px_t
        WINDOW_WIDTH = 300,
        WINDOW_HEIGHT = 32;

    static void sendMessageAndHideWindow(void *thisConfWindow);
};

class InfoWindow : public Window {
public:
    InfoWindow(const std::string &windowText);

private:

    static const px_t
        WINDOW_WIDTH = 300,
        WINDOW_HEIGHT = 32;

    static void deleteWindow(void *thisWindow);
};

#endif
