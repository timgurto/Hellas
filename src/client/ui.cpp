#include <cassert>

#include "../Message.h"
#include "Client.h"
#include "UIGroup.h"
#include "ui/Indicator.h"
#include "ui/OutlinedLabel.h"
#include "ui/TextBox.h"

void Client::showErrorMessage(const std::string &message, Color color) const {
  if (!_lastErrorMessage) return;
  _lastErrorMessage->changeText(message);
  _lastErrorMessage->setColor(color);

  const auto TIME_TO_SHOW_ERROR_MESSAGE = 5000;
  _errorMessageTimer = TIME_TO_SHOW_ERROR_MESSAGE;

  _lastErrorMessage->show();

  _debug(message, color);
}

void Client::showQueuedErrorMessages() {
  for (const auto &msg : _queuedErrorMessagesFromOtherThreads)
    showErrorMessage(msg, Color::CHAT_ERROR);
  _queuedErrorMessagesFromOtherThreads.clear();
}

void Client::initUI() {
  auto properFontHasBeenInitialisedFromConfig = Element::font() != nullptr;
  if (!properFontHasBeenInitialisedFromConfig && _defaultFont)
    TTF_CloseFont(_defaultFont);
  if (!properFontHasBeenInitialisedFromConfig)
    _defaultFont = TTF_OpenFont(_config.fontFile.c_str(), _config.fontSize);

  assert(_defaultFont);
  Element::font(_defaultFont);
  Element::textOffset = _config.fontOffset;
  Element::TEXT_HEIGHT = _config.textHeight;

  Element::initialize();
  ClientItem::init();

  initPerformanceDisplay();
  initChatLog();
  initWindows();
  initializeGearSlotNames();
  initCastBar();
  initHotbar();

  _toolsDisplay = new Element({0, SCREEN_Y - 36, SCREEN_X, 20});
  addUI(_toolsDisplay);

  initBuffsDisplay();
  initMenuBar();
  initPlayerPanels();
  initTargetBuffs();
  initToasts();
  initQuestProgress();

  groupUI = new GroupUI(*this);
  addUI(groupUI->container);
  addUI(groupUI->leaveGroupButton);

  initSkipTutorialButton();

  const auto ERROR_FROM_BOTTOM = 50;
  _lastErrorMessage = new OutlinedLabel(
      {0, SCREEN_Y - ERROR_FROM_BOTTOM, SCREEN_X, Element::TEXT_HEIGHT + 4}, {},
      Element::CENTER_JUSTIFIED);
  _lastErrorMessage->id("Error/warning message");
  _lastErrorMessage->ignoreMouseEvents();
  addUI(_lastErrorMessage);

  const auto INSTRUCTIONS_Y = 100;
  _instructionsLabel =
      new OutlinedLabel({0, INSTRUCTIONS_Y, SCREEN_X, Element::TEXT_HEIGHT + 2},
                        {}, Element::CENTER_JUSTIFIED);
  _instructionsLabel->setColor(Color::UI_FEEDBACK);
  _instructionsLabel->id("Centre-screen Instructions");
  _instructionsLabel->ignoreMouseEvents();
  addUI(_instructionsLabel);
}

void Client::initChatLog() {
  drawLoadingScreen("Initializing chat log");
  _chatContainer =
      new Element({0, SCREEN_Y - _config.chatH, _config.chatW, _config.chatH});
  _chatTextBox = new TextBox(*this, {0, _config.chatH, _config.chatW});
  _chatLog =
      new List({0, 0, _config.chatW, _config.chatH - _chatTextBox->height()});
  _chatTextBox->setPosition(0, _chatLog->height());
  _chatTextBox->hide();

  auto background = new ColorBlock(_chatLog->rect(), Color::CHAT_BACKGROUND);
  background->setAlpha(0x7f);
  _chatContainer->addChild(background);

  _chatContainer->addChild(_chatLog);
  _chatContainer->addChild(_chatTextBox);

  _chatContainer->id("Chat log");
  addUI(_chatContainer);
}

void Client::initWindows() {
  drawLoadingScreen("Initializing UI");

  initializeBuildWindow();
  addWindow(_buildWindow);

  _craftingWindow = Window::InitializeLater(*this, initializeCraftingWindow,
                                            "Crafting (uninitialised)");
  addWindow(_craftingWindow);

  initializeInventoryWindow();
  addWindow(_inventoryWindow);

  _gearWindow = Window::InitializeLater(*this, initializeGearWindow,
                                        "Gear (uninitialised)");
  addWindow(_gearWindow);

  initializeSocialWindow();
  addWindow(_socialWindow);

  initializeMapWindow();
  addWindow(_mapWindow);

  _helpWindow.initialise(*this);
  addWindow(_helpWindow.window);

  initializeClassWindow();
  addWindow(_classWindow);

  initializeEscapeWindow();
  addWindow(_escapeWindow);

  initializeQuestLog();
  addWindow(_questLog);

  _bugReportWindow = new InputWindow(
      *this, "Please provide feedback to the developers:", CL_REPORT_BUG, {},
      TextBox::ALL);
  addWindow(_bugReportWindow);
}

void Client::initCastBar() {
  const ScreenRect CAST_BAR_RECT(SCREEN_X / 2 - _config.castBarW / 2,
                                 _config.castBarY, _config.castBarW,
                                 _config.castBarH),
      CAST_BAR_DIMENSIONS(0, 0, _config.castBarW, _config.castBarH);
  _castBar = new Element(CAST_BAR_RECT);
  _castBar->addChild(
      new ProgressBar<ms_t>(CAST_BAR_DIMENSIONS, _actionTimer, _actionLength));

  for (auto x = -1; x <= 1; ++x)
    for (auto y = -1; y <= 1; ++y) {
      if (x == 0 && y == 0) continue;
      auto rect = CAST_BAR_DIMENSIONS + ScreenRect{x, y};
      auto *labelOutline = new LinkedLabel<std::string>(
          rect, _actionMessage, "", "", Label::CENTER_JUSTIFIED);
      labelOutline->setColor(Color::UI_OUTLINE);
      _castBar->addChild(labelOutline);
    }
  auto *label = new LinkedLabel<std::string>(
      CAST_BAR_DIMENSIONS, _actionMessage, "", "", Label::CENTER_JUSTIFIED);
  _castBar->addChild(label);

  _castBar->hide();
  _castBar->id("Cast bar");
  addUI(_castBar);
}

void Client::initBuffsDisplay() {
  const px_t WIDTH = 60, HEIGHT = 300, ROW_HEIGHT = 20;
  _buffsDisplay = new List({SCREEN_X - WIDTH, 0, WIDTH, HEIGHT}, ROW_HEIGHT);

  _buffsDisplay->id("Player's buffs");
  addUI(_buffsDisplay);

  refreshBuffsDisplay();
}

void Client::refreshBuffsDisplay() {
  _buffsDisplay->clearChildren();

  for (auto buff : _character.buffs())
    _buffsDisplay->addChild(assembleBuffEntry(*this, *buff));
  for (auto buff : _character.debuffs())
    _buffsDisplay->addChild(assembleBuffEntry(*this, *buff, true));

  _buffsDisplay->resizeToContent();
}

Element *Client::assembleBuffEntry(Client &client, const ClientBuffType &type,
                                   bool isDebuff) {
  auto e = new Element();
  const auto ICON_X = client._buffsDisplay->width() - ICON_SIZE - 10;

  auto outlineColor = isDebuff ? Color::DEBUFF : Color::BUFF;
  e->addChild(new ColorBlock({ICON_X - 1, 1, 18, 18}, outlineColor));

  auto icon = new Picture({ICON_X, 2, 16, 16}, type.icon());
  e->addChild(icon);

  if (!isDebuff) {
    icon->setRightMouseDownFunction(
        [&type, &client](Element &e, const ScreenPoint &mousePos) {
          client.sendMessage({CL_DISMISS_BUFF, type.id()});
        });
  }

  auto tooltip = Tooltip{};
  tooltip.setColor(Color::TOOLTIP_NAME);
  tooltip.addLine(type.name());
  if (type.hasDescription()) {
    tooltip.addGap();
    tooltip.setColor();
    tooltip.addLine(type.description());

    if (!isDebuff) {
      tooltip.addGap();
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine("Right-click to dismiss");
    }
  }
  icon->setTooltip(tooltip);

  // Duration display
  auto hasNoDuration = type.duration() == 0;
  if (hasNoDuration) return e;

  auto &timeRemainingMap =
      isDebuff ? client._debuffTimeRemaining : client._buffTimeRemaining;
  auto timeRemainingRect =
      ScreenRect{0, 0, ICON_X - 3, client._buffsDisplay->childHeight()};
  const auto &msRemaining = timeRemainingMap[type.id()];
  for (auto x = -1; x <= 1; ++x)
    for (auto y = -1; y <= 1; ++y) {
      if (x == 0 && y == 0) continue;
      auto outline = new LinkedLabel<ms_t>(
          timeRemainingRect + ScreenRect{x, y}, msRemaining, {}, {},
          Element::RIGHT_JUSTIFIED, Element::CENTER_JUSTIFIED);
      outline->setFormatFunction(msAsShortTimeDisplay);
      outline->setColor(Color::UI_OUTLINE);
      e->addChild(outline);
    }
  auto timeRemainingLabel = new LinkedLabel<ms_t>(
      timeRemainingRect, msRemaining, {}, {}, Element::RIGHT_JUSTIFIED,
      Element::CENTER_JUSTIFIED);
  timeRemainingLabel->setFormatFunction(msAsShortTimeDisplay);
  e->addChild(timeRemainingLabel);

  return e;
}

void Client::initTargetBuffs() {
  const auto &targetPanelRect = _target.panel()->rect();
  const auto GAP = 1_px, X = targetPanelRect.x,
             Y = targetPanelRect.y + targetPanelRect.h + GAP, W = 400, H = 16;
  _targetBuffs = new Element({X, Y, W, H});

  _targetBuffs->id("Target's buffs");
  addUI(_targetBuffs);

  refreshTargetBuffs();
}

void Client::refreshTargetBuffs() {
  _targetBuffs->clearChildren();

  if (!_target.exists()) {
    _targetBuffs->width(0);
    return;
  }

  auto rect = _targetBuffs->rect();
  rect.y = _target.panel()->rect().y + _target.panel()->rect().h + 1;
  _targetBuffs->rect(rect);

  auto x = 0_px;
  for (const auto buff : _target.combatant()->buffs()) {
    auto icon = new Picture({x, 0, 16, 16}, buff->icon());
    icon->setTooltip(buff->name());
    _targetBuffs->addChild(icon);
    x += 17;
  }
  for (const auto buff : _target.combatant()->debuffs()) {
    auto icon = new Picture({x, 0, 16, 16}, buff->icon());

    auto tooltip = Tooltip{};
    tooltip.setColor(Color::TOOLTIP_NAME);
    tooltip.addLine(buff->name());
    if (buff->hasDescription()) {
      tooltip.addGap();
      tooltip.setColor();
      tooltip.addLine(buff->description());
    }
    icon->setTooltip(tooltip);

    _targetBuffs->addChild(icon);
    x += 17;
  }
  auto newWidth = x;
  _targetBuffs->width(newWidth);
}

void Client::initializeEscapeWindow() {
  const auto BUTTON_WIDTH = 80, BUTTON_HEIGHT = 20, GAP = 3,
             WIN_WIDTH = BUTTON_WIDTH + 2 * GAP, NUM_BUTTONS = 2,
             WIN_HEIGHT = NUM_BUTTONS * BUTTON_HEIGHT + (NUM_BUTTONS + 1) * GAP;
  _escapeWindow = Window::WithRectAndTitle({0, 0, WIN_WIDTH, WIN_HEIGHT},
                                           "System Menu"s, mouse());
  _escapeWindow->center();
  auto y = GAP;
  auto x = GAP;

  _escapeWindow->addChild(new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT},
                                     "Exit to desktop"s,
                                     [this]() { exitGame(this); }));
  y += BUTTON_HEIGHT + GAP;

  _escapeWindow->addChild(
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to game"s,
                 [this]() { Window::hideWindow(_escapeWindow); }));
}

void Client::updateUI() {
  if (_timeElapsed >= _errorMessageTimer) {
    _errorMessageTimer = 0;
    _lastErrorMessage->hide();
  } else
    _errorMessageTimer -= _timeElapsed;

  updateToasts();
  refreshTools();
}

void Client::refreshTools() const {
  if (!_currentTools.hasChanged()) return;

  _toolsDisplay->clearChildren();

  const auto numTools = _currentTools.getTools().size();
  const auto totalWidth = px_t{16} * numTools;
  auto x = (_toolsDisplay->width() - totalWidth) / 2;
  for (auto tag : _currentTools.getTools()) {
    auto *picture = new Picture(x, 0, images.toolIcons[tag]);

    // Assumption: no icon = not a tool (e.g., food, shield)
    if (picture->width() != 16) {
      delete picture;
      const auto newRect = _toolsDisplay->rect() + ScreenRect{8, 0};
      _toolsDisplay->rect(newRect);
      continue;
    }

    const auto tagName = gameData.tagName(tag);
    auto article = "a"s;
    if (!tagName.empty()) {
      const auto firstLetter = tagName[0];
      if (firstLetter == 'E' || firstLetter == 'A' || firstLetter == 'O' ||
          firstLetter == 'I' || firstLetter == 'U')
        article = "an";
    }
    picture->setTooltip("You have " + article + " " + tagName + " tool.");

    _toolsDisplay->addChild(picture);
    x += 16;
  }
}

void Client::initMenuBar() {
  static const px_t MENU_BUTTON_W = 12, MENU_BUTTON_H = 12, NUM_BUTTONS = 11,
                    BAR_W = MENU_BUTTON_W * NUM_BUTTONS;
  Element *menuBar = new Element({SCREEN_X - MENU_BUTTON_W * NUM_BUTTONS,
                                  SCREEN_Y - MENU_BUTTON_H,
                                  MENU_BUTTON_W * NUM_BUTTONS, MENU_BUTTON_H});

  addButtonToMenu(menuBar, 0, _buildWindow, "icon-build.png",
                  "Build objects (B)");
  addButtonToMenu(menuBar, 1, _craftingWindow, "icon-crafting.png",
                  "Craft items (C)");
  addButtonToMenu(menuBar, 2, _inventoryWindow, "icon-inventory.png",
                  "Inventory (I)");
  addButtonToMenu(menuBar, 3, _gearWindow, "icon-gear.png", "Gear window (G)");
  addButtonToMenu(menuBar, 4, _classWindow, "icon-class.png", "Talents (K)");
  addButtonToMenu(menuBar, 5, _questLog, "icon-quest.png", "Quest Log (Q)");
  addButtonToMenu(menuBar, 6, _socialWindow, "icon-social.png",
                  "Social information (O)");
  addButtonToMenu(menuBar, 7, _chatContainer, "icon-chat.png",
                  "Toggle chat log (L)");
  addButtonToMenu(menuBar, 8, _mapWindow, "icon-map.png", "World map (M)");
  addButtonToMenu(menuBar, 9, _helpWindow.window, "icon-help.png", "Help (H)");
  addButtonToMenu(menuBar, 10, _bugReportWindow, "icon-bug.png",
                  "Report bug or give feedback");

  menuBar->id("Menu bar");
  addUI(menuBar);
}

void Client::addButtonToMenu(Element *menuBar, size_t index, Element *toToggle,
                             const std::string iconFile,
                             const std::string tooltip) {
  const px_t MENU_BUTTON_W = 12, MENU_BUTTON_H = 12;

  Button *button =
      new Button({MENU_BUTTON_W * static_cast<px_t>(index), 0, MENU_BUTTON_W,
                  MENU_BUTTON_H},
                 "", [toToggle]() { Element::toggleVisibilityOf(toToggle); });
  button->addChild(
      new Picture(2, 2, {"Images/UI/" + iconFile, Color::MAGENTA}));
  button->setTooltip(tooltip);
  menuBar->addChild(button);
}

void Client::initPerformanceDisplay() {
#ifdef _DEBUG
  static const px_t HARDWARE_STATS_W = 100, HARDWARE_STATS_H = 44,
                    HARDWARE_STATS_LABEL_HEIGHT = 11;
  static const ScreenRect HARDWARE_STATS_RECT(
      SCREEN_X - HARDWARE_STATS_W, 0, HARDWARE_STATS_W, HARDWARE_STATS_H);
  Element *hardwareStats = new Element(
      {SCREEN_X - HARDWARE_STATS_W, 0, HARDWARE_STATS_W, HARDWARE_STATS_H});
  LinkedLabel<unsigned> *fps = new LinkedLabel<unsigned>(
      {0, 0, HARDWARE_STATS_W, HARDWARE_STATS_LABEL_HEIGHT}, _fps, "", "fps",
      Label::RIGHT_JUSTIFIED);
  LinkedLabel<ms_t> *lat =
      new LinkedLabel<ms_t>({0, HARDWARE_STATS_LABEL_HEIGHT, HARDWARE_STATS_W,
                             HARDWARE_STATS_LABEL_HEIGHT},
                            _latency, "", "ms", Label::RIGHT_JUSTIFIED);
  LinkedLabel<size_t> *numEntities = new LinkedLabel<size_t>(
      {0, HARDWARE_STATS_LABEL_HEIGHT * 2, HARDWARE_STATS_W,
       HARDWARE_STATS_LABEL_HEIGHT},
      _numEntities, "", " sprites", Label::RIGHT_JUSTIFIED);
  LinkedLabel<int> *channels = new LinkedLabel<int>(
      {0, HARDWARE_STATS_LABEL_HEIGHT * 3, HARDWARE_STATS_W,
       HARDWARE_STATS_LABEL_HEIGHT},
      _channelsPlaying, "", "/" + makeArgs(MIXING_CHANNELS) + " channels",
      Label::RIGHT_JUSTIFIED);
  fps->setColor(Color::DEBUG_TEXT);
  lat->setColor(Color::DEBUG_TEXT);
  numEntities->setColor(Color::DEBUG_TEXT);
  channels->setColor(Color::DEBUG_TEXT);
  hardwareStats->addChild(fps);
  hardwareStats->addChild(lat);
  hardwareStats->addChild(numEntities);
  hardwareStats->addChild(channels);
  hardwareStats->id("Performance stats");
  hardwareStats->ignoreMouseEvents();
  addUI(hardwareStats);
#endif
}

void Client::initPlayerPanels() {
  px_t playerPanelX = CombatantPanel::GAP, playerPanelY = CombatantPanel::GAP;
  CombatantPanel *playerPanel = new CombatantPanel(
      playerPanelX, playerPanelY, CombatantPanel::STANDARD_WIDTH, _username,
      _character.health(), _character.maxHealth(), _character.energy(),
      _character.maxEnergy(), _character.level());
  playerPanel->addXPBar(_xp, _maxXP);
  playerPanel->id("Player panel");
  playerPanel->setLevelColor(Color::DIFFICULTY_NEUTRAL);
  addUI(playerPanel);
  /*
  initializeMenu() must be called before initializePanel(), otherwise the
  right-click menu won't work.
  */
  _target.initializeMenu();
  _target.initializePanel(*this);
  _target.panel()->id("Target-panel context menu");
  addUI(_target.panel());
  _target.menu()->id("Target panel");
  addUI(_target.menu());
}

void Client::initializeQuestLog() {
  _questLog = Window::WithRectAndTitle({50, 50, 250, 100}, "Quests", mouse());
  _questList = new List({0, 0, 250, 100}, 15);
  _questLog->addChild(_questList);
}

void Client::populateQuestLog() {
  const auto NAME_W = 130_px, GAP = 2_px;
  auto BUTTON_W = (_questList->contentWidth() - NAME_W - 4 * GAP) / 2;
  _questList->clearChildren();

  for (auto &pair : gameData.quests) {
    auto &quest = pair.second;
    if (!quest.userIsOnQuest()) continue;

    auto entry = new Element({});
    _questList->addChild(entry);
    auto x = GAP;
    auto questName =
        new Label({x, 0, NAME_W, entry->height()}, quest.nameAndLevel(),
                  Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED);
    questName->setColor(quest.difficultyColor());
    entry->addChild(questName);
    x += NAME_W + GAP;
    entry->addChild(new Button(
        {x, 0, BUTTON_W, entry->height() - GAP}, "Briefing",
        [&quest]() { CQuest::generateWindow(&quest, {}, CQuest::INFO_ONLY); }));
    x += BUTTON_W + GAP;
    auto pQuestID = const_cast<std::string *>(&quest.info().id);
    entry->addChild(new Button({x, 0, BUTTON_W, entry->height() - GAP},
                               "Abandon", [this, pQuestID]() {
                                 sendMessage({CL_ABANDON_QUEST, *pQuestID});
                               }));
  }
}

void Client::initQuestProgress() {
  const auto WIDTH = 152_px, Y = 100_px, HEIGHT = 200_px;
  _questProgress =
      new List{{SCREEN_X - WIDTH, Y, WIDTH, HEIGHT}, Element::TEXT_HEIGHT + 2};
  _questProgress->id("Quest tracker");
  addUI(_questProgress);
  _questProgress->ignoreMouseEvents();
}

void Client::initSkipTutorialButton() {
  const auto W = 100_px, H = 15_px, X = (SCREEN_X - W) / 2,
             Y = SCREEN_Y - H - 20;
  _skipTutorialButton = new Button({X, Y, W, H}, "Skip tutorial", [this]() {
    std::string confirmationText =
        "Are you sure you want to skip the rest of the tutorial?"s;
    if (_skipTutorialConfirmation)
      removeWindow(_skipTutorialConfirmation);
    else
      _skipTutorialConfirmation =
          new ConfirmationWindow(*this, confirmationText, CL_SKIP_TUTORIAL, {});
    addWindow(_skipTutorialConfirmation);
    _skipTutorialConfirmation->show();
  });
  _skipTutorialButton->id("Skip-tutorial button");
  addUI(_skipTutorialButton);
  _skipTutorialButton->hide();
}

void Client::refreshQuestProgress() {
  _questProgress->clearChildren();

  auto isEmpty = true;
  for (const auto &pair : gameData.quests) {
    const auto &quest = pair.second;
    if (!quest.userIsOnQuest()) continue;

    if (!isEmpty) _questProgress->addGap();
    isEmpty = false;

    auto questName = new OutlinedLabel({}, quest.nameInProgressUI());
    questName->setColor(quest.difficultyColor());
    _questProgress->addChild(questName);

    auto allObjectivesCompleted = true;

    for (auto i = 0; i != quest.info().objectives.size(); ++i) {
      const auto &objective = quest.info().objectives[i];

      auto objectiveText = "- "s + objective.text;

      if (objective.qty > 1) {
        objectiveText += " ("s + toString(quest.getProgress(i)) + "/"s +
                         toString(objective.qty) + ")";
      }
      auto objectiveLabel = new OutlinedLabel({}, objectiveText);
      auto objectiveComplete = quest.getProgress(i) == objective.qty;
      objectiveLabel->setColor(objectiveComplete ? Color::UI_DISABLED
                                                 : Color::UI_TEXT);
      _questProgress->addChild(objectiveLabel);

      if (!objectiveComplete) allObjectivesCompleted = false;
    }

    if (allObjectivesCompleted) {
      questName->setColor(Color::UI_DISABLED);
    }
  }

  _questProgress->resizeToContent();
  auto midScreenHeight = (Client::SCREEN_Y - _questProgress->height()) / 2;
  _questProgress->setPosition(Client::SCREEN_X - _questProgress->width(),
                              midScreenHeight);
}
