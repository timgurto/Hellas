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
    Element::absMouse = &_mouse;

    Element::initialize();
    ClientItem::init();

    initChatLog();
    initWindows();
    initializeGearSlotNames();
    initCastBar();
    initHotbar();
    initMenuBar();
    initPerformanceDisplay();
    initPlayerPanels();

}

void Client::initChatLog() {
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

void Client::initWindows() {
    drawLoadingScreen("Initializing UI", 0.8);

    initializeBuildWindow();
    addWindow(_buildWindow);

    _craftingWindow = Window::InitializeLater(initializeCraftingWindow, "Crafting (uninitialised)");
    addWindow(_craftingWindow);

    initializeInventoryWindow();
    addWindow(_inventoryWindow);

    _gearWindow = Window::InitializeLater(initializeGearWindow, "Gear (uninitialised)");
    addWindow(_gearWindow);

    initializeSocialWindow();
    addWindow(_socialWindow);

    initializeMapWindow();
    addWindow(_mapWindow);

    initializeHelpWindow();
    addWindow(_helpWindow);
}

void Client::initCastBar() {
    const Rect
        CAST_BAR_RECT(SCREEN_X / 2 - _config.castBarW / 2, _config.castBarY,
            _config.castBarW, _config.castBarH),
        CAST_BAR_DIMENSIONS(0, 0, _config.castBarW, _config.castBarH);
    static const Color CAST_BAR_LABEL_COLOR = Color::CAST_BAR_FONT;
    _castBar = new Element(CAST_BAR_RECT);
    _castBar->addChild(new ProgressBar<ms_t>(CAST_BAR_DIMENSIONS, _actionTimer, _actionLength));
    LinkedLabel<std::string> *castBarLabel = new LinkedLabel<std::string>(CAST_BAR_DIMENSIONS,
        _actionMessage, "", "",
        Label::CENTER_JUSTIFIED);
    castBarLabel->setColor(CAST_BAR_LABEL_COLOR);
    _castBar->addChild(castBarLabel);
    _castBar->hide();
    addUI(_castBar);
}

void Client::initHotbar() {
    const auto
        NUM_BUTTONS = 10;
    _hotbar = new Element({ 0, 0, NUM_BUTTONS * 18, 18 });
    _hotbar->setPosition((SCREEN_X - _hotbar->width()) / 2, SCREEN_Y - _hotbar->height());
    addUI(_hotbar);
}

void Client::populateHotbar() {
    _hotbar->clearChildren();

    auto x = 0_px;
    for (auto &pair : _spells) {
        const auto &spell = *pair.second;
        void *castMessageVoidPtr = const_cast<void*>(
            reinterpret_cast<const void*>(&spell.castMessage()));
        auto button = new Button({ x, 0, 18, 18 }, {}, sendRawMessageStatic, castMessageVoidPtr);
        button->addChild(new ColorBlock({ 0,  0, 18, 18 }, Color::OUTLINE));
        button->addChild(new Picture(1, 1, spell.icon()));
        _hotbar->addChild(button);

        x += 18;
    }
}

void Client::initMenuBar() {
    static const px_t
        MENU_BUTTON_W = 12,
        MENU_BUTTON_H = 12,
        NUM_BUTTONS = 8;
    Element *menuBar = new Element({ _hotbar->rect().x + _hotbar->width(), SCREEN_Y - MENU_BUTTON_H,
            MENU_BUTTON_W * NUM_BUTTONS, MENU_BUTTON_H });

    addButtonToMenu(menuBar, 0, _buildWindow, "icon-build.png", "Build window (B)");
    addButtonToMenu(menuBar, 1, _craftingWindow, "icon-crafting.png", "Crafting window (C)");
    addButtonToMenu(menuBar, 2, _inventoryWindow, "icon-inventory.png", "Inventory window (I)");
    addButtonToMenu(menuBar, 3, _gearWindow, "icon-gear.png", "Gear window (G)");
    addButtonToMenu(menuBar, 4, _socialWindow, "icon-social.png", "Social window (O)");
    addButtonToMenu(menuBar, 5, _chatContainer, "icon-chat.png", "Toggle chat log");
    addButtonToMenu(menuBar, 6, _mapWindow, "icon-map.png", "Map (M)");
    addButtonToMenu(menuBar, 7, _helpWindow, "icon-help.png", "Help (H)");

    addUI(menuBar);
}

void Client::addButtonToMenu(Element *menuBar, size_t index, Element *toToggle,
        const std::string iconFile, const std::string tooltip) {
    static const px_t
        MENU_BUTTON_W = 12,
        MENU_BUTTON_H = 12;

    Button *button = new Button(Rect(MENU_BUTTON_W * index, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, toToggle);
    button->addChild(new Picture(2, 2, { "Images/UI/" + iconFile, Color::MAGENTA }));
    button->setTooltip(tooltip);
    menuBar->addChild(button);
}

void Client::initPerformanceDisplay() {
    static const px_t
        HARDWARE_STATS_W = 100,
        HARDWARE_STATS_H = 44,
        HARDWARE_STATS_LABEL_HEIGHT = 11;
    static const Rect
        HARDWARE_STATS_RECT(SCREEN_X - HARDWARE_STATS_W, 0, HARDWARE_STATS_W, HARDWARE_STATS_H);
    Element *hardwareStats = new Element(Rect(SCREEN_X - HARDWARE_STATS_W, 0,
        HARDWARE_STATS_W, HARDWARE_STATS_H));
    LinkedLabel<unsigned> *fps = new LinkedLabel<unsigned>(
        Rect(0, 0, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _fps, "", "fps", Label::RIGHT_JUSTIFIED);
    LinkedLabel<ms_t> *lat = new LinkedLabel<ms_t>(
        Rect(0, HARDWARE_STATS_LABEL_HEIGHT, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _latency, "", "ms", Label::RIGHT_JUSTIFIED);
    LinkedLabel<size_t> *numEntities = new LinkedLabel<size_t>(
        Rect(0, HARDWARE_STATS_LABEL_HEIGHT * 2, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _numEntities, "", " sprites", Label::RIGHT_JUSTIFIED);
    LinkedLabel<int> *channels = new LinkedLabel<int>(
        Rect(0, HARDWARE_STATS_LABEL_HEIGHT * 3, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT),
        _channelsPlaying, "", "/" + makeArgs(MIXING_CHANNELS) + " channels", Label::RIGHT_JUSTIFIED);
    fps->setColor(Color::PERFORMANCE_FONT);
    lat->setColor(Color::PERFORMANCE_FONT);
    numEntities->setColor(Color::PERFORMANCE_FONT);
    channels->setColor(Color::PERFORMANCE_FONT);
    hardwareStats->addChild(fps);
    hardwareStats->addChild(lat);
    hardwareStats->addChild(numEntities);
    hardwareStats->addChild(channels);
    addUI(hardwareStats);
}

void Client::initPlayerPanels() {
    px_t
        playerPanelX = CombatantPanel::GAP,
        playerPanelY = CombatantPanel::GAP;
    CombatantPanel *playerPanel = new CombatantPanel(playerPanelX, playerPanelY, _username,
        _character.health(), _character.maxHealth());
    playerPanel->changeColor(Color::COMBATANT_SELF);
    addUI(playerPanel);
    /*
    initializeMenu() must be called before initializePanel(), otherwise the right-click menu won't
    work.
    */
    _target.initializeMenu();
    _target.initializePanel();
    addUI(_target.panel());
    addUI(_target.menu());
}
