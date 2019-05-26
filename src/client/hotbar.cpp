#include "Client.h"

void Client::initHotbar() {
  _hotbar = new Element({0, 0, NUM_HOTBAR_BUTTONS * 18, 18});
  _hotbar->setPosition((SCREEN_X - _hotbar->width()) / 2,
                       SCREEN_Y - _hotbar->height());
  addUI(_hotbar);
}

void Client::populateHotbar() {
  _hotbar->clearChildren();

  for (auto i = 0; i != NUM_HOTBAR_BUTTONS; ++i) {
    auto button = new Button({i * 18, 0, 18, 18});
    static const auto HOTKEY_GLYPHS = std::vector<std::string>{
        "'", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};
    button->addChild(new OutlinedLabel({0, -1, 15, 18}, HOTKEY_GLYPHS[i],
                                       Element::RIGHT_JUSTIFIED));
    _hotbar->addChild(button);
    _hotbarButtons[i] = button;
  }
}
