#include <cassert>

#include "Client.h"
#include "ui/TextBox.h"

void Client::initUI() {
    if (_defaultFont != nullptr)
        TTF_CloseFont(_defaultFont);
    _defaultFont = TTF_OpenFont(_config.fontFile.c_str(), _config.fontSize);
    assert(_defaultFont != nullptr);
    Element::font(_defaultFont);
    Element::textOffset = _config.fontOffset;
    Element::TEXT_HEIGHT = _config.textHeight;

    Element::initialize();
    ClientItem::init();

    // Initialize chat log
    drawLoadingScreen("Initializing chat log", 0.4);
    _chatContainer = new Element(Rect(0, SCREEN_Y - _config.chatH, _config.chatW, _config.chatH));
    _chatTextBox = new TextBox(Rect(0, _config.chatH, _config.chatW));
    _chatLog = new List(Rect(0, 0, _config.chatW, _config.chatH - _chatTextBox->height()));
    _chatTextBox->setPosition(0, _chatLog->height());
    _chatTextBox->hide();
    _chatContainer->addChild(new ColorBlock(_chatLog->rect(), Color::CHAT_LOG_BACKGROUND));
    _chatContainer->addChild(_chatLog);
    _chatContainer->addChild(_chatTextBox);

    addUI(_chatContainer);
    SAY_COLOR = Color::SAY;
    WHISPER_COLOR = Color::WHISPER;
}
