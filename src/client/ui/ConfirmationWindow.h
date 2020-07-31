#ifndef CONFIRMATION_WINDOW_H
#define CONFIRMATION_WINDOW_H

#include "../../messageCodes.h"
#include "Element.h"
#include "TextBox.h"
#include "Window.h"

class DialogWindow : public Window {
 public:
  DialogWindow(Client &client) { setClient(client); }
};

class ConfirmationWindow : public DialogWindow {
 public:
  ConfirmationWindow(Client &client, const std::string &windowText,
                     MessageCode msgCode, const std::string &msgArgs);

 private:
  MessageCode _msgCode;
  std::string _msgArgs;

  static const px_t WINDOW_WIDTH = 300, WINDOW_HEIGHT = 32;

  static void sendMessageAndHideWindow(void *thisConfWindow);
};

class InfoWindow : public DialogWindow {
 public:
  InfoWindow(Client &client, const std::string &windowText);

 private:
  static const px_t WINDOW_WIDTH = 300, WINDOW_HEIGHT = 32;

  static void deleteWindow(void *thisWindow);
};

class InputWindow : public DialogWindow {
 public:
  InputWindow(Client &client, const std::string &windowText,
              MessageCode msgCode, const std::string &msgArgs,
              TextBox::ValidInput validInput);

 private:
  MessageCode _msgCode;
  std::string _msgArgs;
  TextBox *_textBox;

  static void sendMessageWithInputAndHideWindow(void *thisInputWindow);
};

#endif
