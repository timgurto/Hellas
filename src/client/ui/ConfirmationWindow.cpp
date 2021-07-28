#include "ConfirmationWindow.h"

#include "../../Message.h"
#include "../Client.h"
#include "Label.h"

ConfirmationWindow::ConfirmationWindow(Client &client,
                                       const std::string &windowText,
                                       MessageCode msgCode,
                                       const std::string &msgArgs)
    : DialogWindow(client), _msgCode(msgCode), _msgArgs(msgArgs) {
  resize(WINDOW_WIDTH, WINDOW_HEIGHT);
  center();
  setTitle("Confirmation");

  static const px_t PADDING = 2, BUTTON_WIDTH = 60, BUTTON_HEIGHT = 15,
                    BUTTON_Y = 2 * PADDING + Element::TEXT_HEIGHT;

  addChild(new Label({0, PADDING, WINDOW_WIDTH, Element::TEXT_HEIGHT},
                     windowText, Element::CENTER_JUSTIFIED));
  px_t middle = WINDOW_WIDTH / 2,
       okButtonX = middle - PADDING / 2 - BUTTON_WIDTH,
       cancelButtonX = middle + PADDING / 2;
  addChild(new Button({okButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT}, "OK",
                      [this]() { sendMessageAndHideWindow(this); }));
  addChild(new Button({cancelButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
                      "Cancel", [this]() { Window::hideWindow(this); }));
}

void ConfirmationWindow::sendMessageAndHideWindow(void *thisConfWindow) {
  ConfirmationWindow *window =
      reinterpret_cast<ConfirmationWindow *>(thisConfWindow);
  window->client()->sendMessage({window->_msgCode, window->_msgArgs});
  window->hide();
}

InfoWindow::InfoWindow(Client &client, const std::string &windowText)
    : DialogWindow(client) {
  resize(WINDOW_WIDTH, WINDOW_HEIGHT);
  center();
  setTitle("Info");

  static const px_t PADDING = 2, BUTTON_WIDTH = 60, BUTTON_HEIGHT = 15,
                    BUTTON_Y = 2 * PADDING + Element::TEXT_HEIGHT;

  addChild(new Label({0, PADDING, WINDOW_WIDTH, Element::TEXT_HEIGHT},
                     windowText, Element::CENTER_JUSTIFIED));
  px_t middle = WINDOW_WIDTH / 2, okButtonX = middle - BUTTON_WIDTH / 2;
  addChild(new Button({okButtonX, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT}, "OK",
                      [this]() { hideWindow(this); }));
}

void InfoWindow::deleteWindow(void *thisWindow) {
  auto *window = reinterpret_cast<Window *>(thisWindow);
  window->client()->removeWindow(window);
}

InputWindow::InputWindow(Client &client, const std::string &windowText,
                         MessageCode msgCode, const std::string &msgArgs,
                         TextBox::ValidInput validInput)
    : DialogWindow(client), _msgCode(msgCode), _msgArgs(msgArgs) {
  const px_t WINDOW_WIDTH = 300, WINDOW_HEIGHT = 48, PADDING = 2,
             BUTTON_WIDTH = 60, BUTTON_HEIGHT = 15;

  resize(WINDOW_WIDTH, WINDOW_HEIGHT);
  center();
  setTitle("Input");

  auto y = PADDING;
  addChild(new Label({0, y, WINDOW_WIDTH, Element::TEXT_HEIGHT}, windowText,
                     Element::CENTER_JUSTIFIED));
  y += Element::TEXT_HEIGHT + PADDING;

  _textBox = new TextBox(client, {0, y, WINDOW_WIDTH, Element::TEXT_HEIGHT},
                         validInput);
  addChild(_textBox);
  y += _textBox->height() + PADDING;

  px_t middle = WINDOW_WIDTH / 2,
       okButtonX = middle - PADDING / 2 - BUTTON_WIDTH,
       cancelButtonX = middle + PADDING / 2;
  addChild(new Button({okButtonX, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "OK",
                      [this]() { sendMessageWithInputAndHideWindow(this); }));
  addChild(new Button({cancelButtonX, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Cancel",
                      [this]() { Window::hideWindow(this); }));
}

void InputWindow::sendMessageWithInputAndHideWindow(void *thisInputWindow) {
  auto *window = reinterpret_cast<InputWindow *>(thisInputWindow);
  auto args = window->_textBox->text();
  if (!window->_msgArgs.empty()) args = makeArgs(window->_msgArgs, args);
  window->client()->sendMessage({window->_msgCode, args});
  window->hide();
  window->_textBox->text({});
}

DialogWindow::DialogWindow(Client &client) : Window(client.mouse()) {
  setClient(client);
}
