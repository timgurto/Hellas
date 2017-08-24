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

    // Initialize chat log
    initChatLog();
    initializeGearSlotNames();
    initCastBar();
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
    _craftingWindow = Window::InitializeLater(initializeCraftingWindow);
    initializeInventoryWindow();
    _gearWindow = Window::InitializeLater(initializeGearWindow);
    initializeMapWindow();
    addWindow(_buildWindow);
    addWindow(_craftingWindow);
    addWindow(_inventoryWindow);
    addWindow(_gearWindow);
    addWindow(_mapWindow);
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

void Client::initMenuBar() {
    static const px_t
        MENU_BUTTON_W = 12,
        MENU_BUTTON_H = 12,
        NUM_BUTTONS = 6;
    Element *menuBar = new Element(Rect(SCREEN_X / 2 - MENU_BUTTON_W * NUM_BUTTONS / 2,
        SCREEN_Y - MENU_BUTTON_H,
        MENU_BUTTON_W * NUM_BUTTONS,
        MENU_BUTTON_H));
    Button *button = new Button(Rect(0, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _buildWindow);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-build.png", Color::MAGENTA }));
    button->setTooltip("Build window (B)");
    menuBar->addChild(button);
    button = new Button(Rect(MENU_BUTTON_W, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _craftingWindow);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-crafting.png", Color::MAGENTA }));
    button->setTooltip("Crafting window (C)");
    menuBar->addChild(button);
    button = new Button(Rect(MENU_BUTTON_W * 2, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _inventoryWindow);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-inventory.png", Color::MAGENTA }));
    button->setTooltip("Inventory window (I)");
    menuBar->addChild(button);
    button = new Button(Rect(MENU_BUTTON_W * 3, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _gearWindow);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-gear.png", Color::MAGENTA }));
    button->setTooltip("Gear window (G)");
    menuBar->addChild(button);
    button = new Button(Rect(MENU_BUTTON_W * 4, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _chatContainer);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-chat.png", Color::MAGENTA }));
    button->setTooltip("Toggle chat log");
    menuBar->addChild(button);
    button = new Button(Rect(MENU_BUTTON_W * 5, 0, MENU_BUTTON_W, MENU_BUTTON_H), "",
        Element::toggleVisibilityOf, _mapWindow);
    button->addChild(new Picture(2, 2, { "Images/UI/icon-map.png", Color::MAGENTA }));
    button->setTooltip("Map (M)");
    menuBar->addChild(button);
    addUI(menuBar);
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
