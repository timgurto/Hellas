#include "ClientObject.h"

#include <SDL_mixer.h>

#include <cassert>

#include "../Color.h"
#include "../Log.h"
#include "../util.h"
#include "Client.h"
#include "ClientNPC.h"
#include "ClientVehicle.h"
#include "Particle.h"
#include "Renderer.h"
#include "Tooltip.h"
#include "Unlocks.h"
#include "ui/Button.h"
#include "ui/ConfirmationWindow.h"
#include "ui/ContainerGrid.h"
#include "ui/ItemSelector.h"
#include "ui/Label.h"
#include "ui/Line.h"
#include "ui/TakeContainer.h"
#include "ui/TextBox.h"
#include "ui/Window.h"

extern Renderer renderer;

const px_t ClientObject::BUTTON_HEIGHT = 15;
const px_t ClientObject::BUTTON_WIDTH = 60;
const px_t ClientObject::GAP = 2;
const px_t ClientObject::BUTTON_GAP = 1;

ClientObject::ClientObject(const ClientObject &rhs)
    : Sprite(rhs),
      ClientCombatant(rhs.objectType()),
      _serial(rhs._serial),
      _owner({Owner::ALL_HAVE_ACCESS, {}}),
      _container(rhs._container),
      _window(nullptr),
      _confirmCedeWindow(nullptr),
      _beingGathered(rhs._beingGathered) {}

ClientObject::ClientObject(Serial serialArg, const ClientObjectType *type,
                           const MapPoint &loc)
    : Sprite(type, loc),
      ClientCombatant(type),
      _serial(serialArg),
      _owner({Owner::ALL_HAVE_ACCESS, {}}),
      _window(nullptr),
      _confirmCedeWindow(nullptr),
      _beingGathered(false),
      _dropbox(1),
      _gatherSoundTimer(0),
      _lootable(false),
      _lootContainer(nullptr) {
  if (type == nullptr)  // i.e., a serial-only search dummy
    return;

  _transformTimer = type->transformTime();

  const size_t containerSlots = objectType()->containerSlots(),
               merchantSlots = objectType()->merchantSlots();
  _container = ClientItem::vect_t(containerSlots);
  _merchantSlots = std::vector<ClientMerchantSlot>(merchantSlots);
  _merchantSlotElements = std::vector<Element *>(merchantSlots, nullptr);
  _wareQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
  _priceQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
}

ClientObject::~ClientObject() {
  if (_window != nullptr) {
    Client::_instance->removeWindow(_window);
    delete _window;
  }
  if (_confirmCedeWindow != nullptr) {
    Client::_instance->removeWindow(_confirmCedeWindow);
    delete _confirmCedeWindow;
  }
  if (_grantWindow != nullptr) {
    Client::_instance->removeWindow(_grantWindow);
    delete _grantWindow;
  }
}

bool ClientObject::isOwnedByPlayer(const std::string &playerName) const {
  return _owner.type == Owner::PLAYER && _owner.name == playerName;
}

bool ClientObject::isOwnedByCity(const std::string &cityName) const {
  return _owner.type == Owner::CITY && _owner.name == cityName;
}

void ClientObject::setMerchantSlot(size_t i, ClientMerchantSlot &mSlotArg) {
  _merchantSlots[i] = mSlotArg;
  ClientMerchantSlot &mSlot = _merchantSlots[i];

  if (_window == nullptr || isBeingConstructed()) return;

  // Uninitialised element probably means we don't have permission to use the
  // slot anyway.
  if (!_merchantSlotElements[i]) return;

  // Update slot element
  Element &e = *_merchantSlotElements[i];
  const ClientItem *wareItem = toClientItem(mSlot.wareItem),
                   *priceItem = toClientItem(mSlot.priceItem);
  e.clearChildren();

  static const px_t  // TODO: remove duplicate consts
      NAME_WIDTH = 100,
      QUANTITY_WIDTH = 30, BUTTON_PADDING = 1,
      TEXT_HEIGHT = Element::TEXT_HEIGHT, ICON_SIZE = Element::ITEM_HEIGHT,
      BUTTON_LABEL_WIDTH = 30 + QUANTITY_WIDTH,
      BUTTON_HEIGHT = ICON_SIZE + 2 * BUTTON_PADDING, BUTTON_TOP = GAP + 2,
      BUTTON_WIDTH =
          BUTTON_PADDING * 2 + BUTTON_LABEL_WIDTH + ICON_SIZE + NAME_WIDTH,
      ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
      TEXT_TOP = (ROW_HEIGHT - TEXT_HEIGHT) / 2,
      BUTTON_TEXT_TOP = (BUTTON_HEIGHT - TEXT_HEIGHT) / 2,
      SET_BUTTON_WIDTH = 60, SET_BUTTON_HEIGHT = 15,
      SET_BUTTON_TOP = (ROW_HEIGHT - SET_BUTTON_HEIGHT) / 2;

  if (userHasAccess()) {  // Setup view
    px_t x = GAP;
    TextBox *textBox = new TextBox({x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT},
                                   TextBox::NUMERALS);
    _wareQtyBoxes[i] = textBox;
    textBox->text(toString(mSlot.wareQty));
    e.addChild(textBox);
    x += QUANTITY_WIDTH + GAP;
    e.addChild(new ItemSelector(mSlot.wareItem, x, BUTTON_TOP));
    x += ICON_SIZE + 2 + NAME_WIDTH + 3 * GAP + 2;
    textBox = new TextBox({x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT},
                          TextBox::NUMERALS);
    _priceQtyBoxes[i] = textBox;
    textBox->text(toString(mSlot.priceQty));
    e.addChild(textBox);
    x += QUANTITY_WIDTH + GAP;
    e.addChild(new ItemSelector(mSlot.priceItem, x, BUTTON_TOP));
    x += ICON_SIZE + 2 + NAME_WIDTH + 2 * GAP + 2;
    e.addChild(
        new Button({x, SET_BUTTON_TOP, SET_BUTTON_WIDTH, SET_BUTTON_HEIGHT},
                   "Set", [this, i]() { sendMerchantSlot(serial(), i); }));

  } else {  // Trade view
    if (!mSlot) {
      refreshTooltip();
      return;  // Blank
    }

    // Ware
    px_t x = GAP;
    e.addChild(new Label({x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT},
                         toString(mSlot.wareQty), Element::RIGHT_JUSTIFIED));
    x += QUANTITY_WIDTH;
    auto icon =
        new Picture({x, (ROW_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE},
                    wareItem->icon());
    icon->setTooltip(wareItem->tooltip());
    e.addChild(icon);

    x += ICON_SIZE;
    e.addChild(
        new Label({x, TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT}, wareItem->name()));
    x += NAME_WIDTH + GAP;

    // Buy button
    Button *button = new Button({x, GAP, BUTTON_WIDTH, BUTTON_HEIGHT}, "",
                                [this, i]() { trade(serial(), i); });
    e.addChild(button);
    x = BUTTON_PADDING;
    button->addChild(
        new Label({x, BUTTON_TEXT_TOP, BUTTON_LABEL_WIDTH, TEXT_HEIGHT},
                  std::string("Buy for ") + toString(mSlot.priceQty),
                  Element::RIGHT_JUSTIFIED));
    x += BUTTON_LABEL_WIDTH;
    button->addChild(new Picture({x, BUTTON_PADDING, ICON_SIZE, ICON_SIZE},
                                 priceItem->icon()));
    x += ICON_SIZE;
    button->addChild(new Label({x, BUTTON_TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT},
                               priceItem->name()));
  }

  refreshTooltip();
}

void ClientObject::onLeftClick(Client &client) {
  if (client.isCtrlPressed()) {
    const auto *npc = dynamic_cast<ClientNPC *>(this);
    if (npc && npc->canBeTamed()) client.sendMessage({CL_TAME_NPC, serial()});

  } else if (client.isAltPressed()) {
    if (objectType()->repairInfo().canBeRepaired)
      client.sendMessage({CL_REPAIR_OBJECT, serial()});
  }

  else
    client.setTarget(*this);

  // Note: parent class's onLeftClick() not called.
}

void ClientObject::onRightClick(Client &client) {
  client.setTarget(*this, canBeAttackedByPlayer());

  // Make sure object is in range
  auto relevantRange = Client::ACTION_DISTANCE;
  if (canBeAttackedByPlayer()) {
    const auto *weapon =
        Client::instance().character().gear()[Item::WEAPON_SLOT].first.type();
    if (weapon) relevantRange = weapon->weaponRange();
  }
  if (distance(client.playerCollisionRect(), collisionRect()) > relevantRange) {
    client.showErrorMessage("That object is too far away.",
                            Color::CHAT_WARNING);
    return;
  }

  // Gatherable
  const ClientObjectType &objType = *objectType();

  const auto canGather =
      objType.canGather() && userHasAccess() && !isBeingConstructed();
  if (canGather) {
    client.sendMessage({CL_GATHER, _serial});
    client.prepareAction(std::string("Gathering from ") + objType.name());
    return;
  }

  // Bring existing window to front
  if (_window != nullptr) {
    client.removeWindow(_window);
    client.addWindow(_window);
  }

  // Create window, if necessary
  else {
    assembleWindow(client);
    if (_window) client.addWindow(_window);
  }

  if (_window != nullptr) {
    // Determine placement: below object, but keep entirely on screen.
    px_t x =
        toInt(location().x - _window->Element::width() / 2 + client.offset().x);
    x = max(0, min(x, Client::SCREEN_X - _window->Element::width()));
    static const px_t WINDOW_GAP_FROM_OBJECT = 20;
    px_t y =
        toInt(location().y + WINDOW_GAP_FROM_OBJECT / 2 + client.offset().y);
    y = max(0, min(y, Client::SCREEN_Y - _window->Element::height()));
    _window->setPosition(x, y);

    _window->show();
  }
}

void ClientObject::addQuestsToWindow() {
  const Client &client = Client::instance();
  px_t y = _window->contentHeight(), newWidth = _window->contentWidth();

  const auto BUTTON_HEIGHT = 15, BUTTON_WIDTH = 150, MARGIN = 2;

  static auto startQuestIcon =
      Texture{"Images/UI/startQuest.png", Color::MAGENTA};
  static auto endQuestIcon = Texture{"Images/UI/endQuest.png", Color::MAGENTA};
  const auto BUTTON_X = startQuestIcon.width();

  auto questsToDisplay = std::map<CQuest *, bool>{};  // true=start, false=end
  for (auto quest : startsQuests()) questsToDisplay[quest] = true;
  for (auto quest : completableQuests()) questsToDisplay[quest] = false;

  for (auto pair : questsToDisplay) {
    auto quest = pair.first;
    auto startsThisQuest = pair.second;
    y += MARGIN;

    auto &icon = startsThisQuest ? startQuestIcon : endQuestIcon;
    _window->addChild(
        new Picture(0, y + (BUTTON_HEIGHT - icon.height()) / 2, icon));

    const auto buttonRect =
        ScreenRect{BUTTON_X, y, BUTTON_WIDTH, BUTTON_HEIGHT};
    auto data = makeArgs(quest->info().id, serial());
    auto pendingState = startsThisQuest ? CQuest::ACCEPT : CQuest::COMPLETE;
    auto button = new Button(buttonRect, quest->info().name, [=]() {
      CQuest::generateWindow(quest, serial(), pendingState);
    });
    button->id(quest->info().id);
    _window->addChild(button);

    y += BUTTON_HEIGHT;
  }

  y += MARGIN;

  newWidth = max(newWidth, BUTTON_WIDTH + BUTTON_X + MARGIN);
  _window->resize(newWidth, y);
}

void ClientObject::addConstructionToWindow() {
  px_t x = 0, y = _window->contentHeight(), newWidth = _window->contentWidth();
  static const px_t LABEL_W = 140;

  _window->addChild(
      new Label({x, y, LABEL_W, Element::TEXT_HEIGHT}, "Under construction"));
  if (newWidth < LABEL_W) newWidth = LABEL_W;
  y += Element::TEXT_HEIGHT + GAP;

  // 1. Required materials
  _window->addChild(new Label({x, y, LABEL_W, Element::TEXT_HEIGHT},
                              "Remaining materials required:"));
  y += Element::TEXT_HEIGHT;
  for (const auto &pair : constructionMaterials()) {
    // Quantity
    static const px_t QTY_WIDTH = 20;
    _window->addChild(new Label({x, y, QTY_WIDTH, Client::ICON_SIZE},
                                makeArgs(pair.second), Element::RIGHT_JUSTIFIED,
                                Element::CENTER_JUSTIFIED));
    x += QTY_WIDTH + GAP;
    // Icon
    const ClientItem &item = *dynamic_cast<const ClientItem *>(pair.first);
    _window->addChild(new Picture(x, y, item.icon()));
    x += Client::ICON_SIZE + GAP;
    // Name
    _window->addChild(new Label({x, y, LABEL_W, Client::ICON_SIZE}, item.name(),
                                Element::LEFT_JUSTIFIED,
                                Element::CENTER_JUSTIFIED));
    y += Client::ICON_SIZE + GAP;
    if (newWidth < x) newWidth = x;
    x = BUTTON_GAP;
  }

  // 2. Dropbox
  static const px_t DROPBOX_LABEL_W = 70;
  ContainerGrid *dropbox = new ContainerGrid(1, 1, _dropbox, _serial, x, y);
  _window->addChild(new Label({x, y, DROPBOX_LABEL_W, dropbox->height()},
                              "Add materials:", Element::RIGHT_JUSTIFIED,
                              Element::CENTER_JUSTIFIED));
  x += DROPBOX_LABEL_W + GAP;
  dropbox->setPosition(x, y);
  _window->addChild(dropbox);
  x += dropbox->width() + GAP;

  // 3. Auto-fill button
  const auto AUTO_BUTTON_W = 50_px, AUTO_BUTTON_H = 15_px;
  const auto buttonY = y + (dropbox->height() - AUTO_BUTTON_H) / 2;
  auto autoFillButton =
      new Button({x, buttonY, AUTO_BUTTON_W, AUTO_BUTTON_H}, "Auto", [this]() {
        Client::instance().sendMessage({CL_AUTO_CONSTRUCT, makeArgs(serial())});
      });
  autoFillButton->setTooltip(
      "Add all required materials that you have in your inventory.  This "
      "doesn't include materials that return items when used.");
  _window->addChild(autoFillButton);
  x += AUTO_BUTTON_W + GAP;
  if (newWidth < x) newWidth = x;

  y += dropbox->height() + GAP;

  _window->resize(newWidth, y);
}

void ClientObject::addMerchantSetupToWindow() {
  px_t x = 0, y = _window->contentHeight(), newWidth = _window->contentWidth();

  static const px_t QUANTITY_WIDTH = 20, NAME_WIDTH = 100,
                    PANE_WIDTH = Element::ITEM_HEIGHT + QUANTITY_WIDTH +
                                 NAME_WIDTH + 2 * GAP,
                    TITLE_HEIGHT = 14, SET_BUTTON_WIDTH = 60,
                    ROW_HEIGHT = Element::ITEM_HEIGHT + 4 * GAP;
  static const double MAX_ROWS = 5.5;
  const px_t LIST_HEIGHT =
      toInt(ROW_HEIGHT * min(MAX_ROWS, objectType()->merchantSlots()));
  x = GAP;
  _window->addChild(new Label({x, y, PANE_WIDTH, TITLE_HEIGHT}, "Item to sell",
                              Element::CENTER_JUSTIFIED));
  x += PANE_WIDTH + GAP;
  Line *vertDivider =
      new Line(x, y, TITLE_HEIGHT + LIST_HEIGHT, Line::VERTICAL);
  _window->addChild(vertDivider);
  x += vertDivider->width() + GAP;
  _window->addChild(new Label({x, y, PANE_WIDTH, TITLE_HEIGHT}, "Price",
                              Element::CENTER_JUSTIFIED));
  x += PANE_WIDTH + GAP;
  vertDivider = new Line(x, y, TITLE_HEIGHT + LIST_HEIGHT, Line::VERTICAL);
  _window->addChild(vertDivider);
  x += 2 * GAP + vertDivider->width() + SET_BUTTON_WIDTH;
  x += List::ARROW_W;
  if (newWidth < x) newWidth = x;
  y += Element::TEXT_HEIGHT;
  List *merchantList = new List(
      {0, y, PANE_WIDTH * 2 + GAP * 5 + SET_BUTTON_WIDTH + 6 + List::ARROW_W,
       LIST_HEIGHT},
      ROW_HEIGHT);
  _window->addChild(merchantList);
  y += LIST_HEIGHT;

  for (size_t i = 0; i != objectType()->merchantSlots(); ++i) {
    _merchantSlotElements[i] = new Element();
    setMerchantSlot(i, ClientMerchantSlot{});
    merchantList->addChild(_merchantSlotElements[i]);
  }

  _window->resize(newWidth, y);
}

void ClientObject::addInventoryToWindow() {
  px_t y = _window->contentHeight(), newWidth = _window->contentWidth();

  const size_t slots = objectType()->containerSlots();
  static const size_t COLS = 8;
  size_t rows = (slots - 1) / COLS + 1;
  ContainerGrid *container =
      new ContainerGrid(rows, COLS, _container, _serial, 0, y);
  _window->addChild(container);
  y += container->height();
  if (newWidth < container->width()) newWidth = container->width();

  _window->resize(newWidth, y);
}

void ClientObject::addDeconstructionToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *deconstructButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Pick up",
                 [this]() { startDeconstructing(this); });
  _window->addChild(deconstructButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::addActionToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  const auto &action = objectType()->action();

  // Description
  const auto DESCRIPTION_WIDTH = 150_px;
  auto lines =
      WordWrapper{Element::font(), DESCRIPTION_WIDTH}.wrap(action.tooltip);
  for (auto line : lines) {
    x = BUTTON_GAP;
    _window->addChild(
        new Label({x, y, DESCRIPTION_WIDTH, Element::TEXT_HEIGHT}, line));
    x += DESCRIPTION_WIDTH;
    y += Element::TEXT_HEIGHT;
    if (newWidth < x) newWidth = x;
  }

  if (action.cost) {
    x = BUTTON_GAP;
    _window->addChild(new Label({x, y, DESCRIPTION_WIDTH, Element::ITEM_HEIGHT},
                                "Consumes: ", Element::LEFT_JUSTIFIED,
                                Element::CENTER_JUSTIFIED));
    x += 45;
    _window->addChild(new Picture(x, y, action.cost->icon()));
    x += Element::ITEM_HEIGHT;
    _window->addChild(new Label({x, y, DESCRIPTION_WIDTH, Element::ITEM_HEIGHT},
                                action.cost->name(), Element::LEFT_JUSTIFIED,
                                Element::CENTER_JUSTIFIED));
    y += Element::ITEM_HEIGHT;
  }

  // Text input
  x = BUTTON_GAP;
  if (!action.textInput.empty()) {
    const auto LABEL_WIDTH = px_t{50};
    auto *label = new Label({x, y, LABEL_WIDTH, 13}, action.textInput);
    _window->addChild(label);
    x += LABEL_WIDTH + BUTTON_GAP;

    // Assumptions: single word, with capital initial.
    _actionTextEntry = new TextBox({x, y, 100, 13}, TextBox::LETTERS);
    _actionTextEntry->forcePascalCase();

    _window->addChild(_actionTextEntry);
    y += 13 + BUTTON_GAP;
    x += 50;
    if (newWidth < x) newWidth = x;
  }

  // Button
  x = BUTTON_GAP;
  Button *button = new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, action.label,
                              [this]() { performAction(this); });
  _window->addChild(button);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::performAction(void *object) {
  assert(object != nullptr);
  ClientObject &obj = *reinterpret_cast<ClientObject *>(object);
  Client &client = *Client::_instance;

  auto textArg = std::string{"_"};
  if (obj._actionTextEntry != nullptr) textArg = obj._actionTextEntry->text();
  if (textArg.empty()) textArg = "_";

  client.sendMessage(
      {CL_PERFORM_OBJECT_ACTION, makeArgs(obj.serial(), textArg)});
}

void ClientObject::addMerchantTradeToWindow() {
  px_t y = _window->contentHeight(), newWidth = _window->contentWidth();

  static const px_t NAME_WIDTH = 100, QUANTITY_WIDTH = 30, BUTTON_PADDING = 1,
                    BUTTON_LABEL_WIDTH = 45,
                    BUTTON_HEIGHT = Element::ITEM_HEIGHT + 2 * BUTTON_PADDING,
                    ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
                    WIDTH = 2 * Element::ITEM_HEIGHT + 2 * NAME_WIDTH +
                            QUANTITY_WIDTH + BUTTON_LABEL_WIDTH +
                            2 * BUTTON_PADDING + 3 * GAP + List::ARROW_W;
  const double MAX_ROWS = 7.5;
  const double NUM_ROWS = min(objectType()->merchantSlots(), MAX_ROWS);
  const px_t HEIGHT = toInt(ROW_HEIGHT * NUM_ROWS);
  List *list = new List({0, 0, WIDTH, HEIGHT}, ROW_HEIGHT);
  y += list->height();
  _window->addChild(list);
  for (size_t i = 0; i != objectType()->merchantSlots(); ++i) {
    _merchantSlotElements[i] = new Element();
    list->addChild(_merchantSlotElements[i]);
  }
  if (newWidth < WIDTH) newWidth = WIDTH;

  _window->resize(newWidth, y);
}

void ClientObject::addCedeButtonToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *cedeButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Cede to city",
                 [this]() { confirmAndCedeObject(this); });
  cedeButton->setTooltip("Transfer ownership of this object over to your city");
  _window->addChild(cedeButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::confirmAndCedeObject(void *objectToCede) {
  assert(objectToCede != nullptr);
  ClientObject &obj = *reinterpret_cast<ClientObject *>(objectToCede);
  Client &client = *Client::_instance;
  std::string confirmationText =
      "Are you sure you want to cede this " + obj.name() + " to your city?";
  if (obj._confirmCedeWindow != nullptr)
    client.removeWindow(obj._confirmCedeWindow);
  else
    obj._confirmCedeWindow = new ConfirmationWindow(confirmationText, CL_CEDE,
                                                    makeArgs(obj.serial()));
  client.addWindow(obj._confirmCedeWindow);
  obj._confirmCedeWindow->show();
}

void ClientObject::addGrantButtonToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *grantButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Grant to citizen",
                 [this]() { getInputAndGrantObject(this); });
  grantButton->setTooltip(
      "Transfer ownership of this object over to a member of your city");
  _window->addChild(grantButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::getInputAndGrantObject(void *objectToGrant) {
  assert(objectToGrant != nullptr);
  ClientObject &obj = *reinterpret_cast<ClientObject *>(objectToGrant);
  Client &client = *Client::_instance;
  auto windowText = "Please enter the name of the new owner:"s;
  if (obj._grantWindow != nullptr)
    client.removeWindow(obj._grantWindow);
  else
    obj._grantWindow = new InputWindow(
        windowText, CL_GRANT, makeArgs(obj.serial()), TextBox::LETTERS);
  client.addWindow(obj._grantWindow);
  obj._grantWindow->show();
}

void ClientObject::addDemolishButtonToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *demolishButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, demolishButtonText(),
                 [this]() { confirmAndDemolishObject(this); });
  demolishButton->setTooltip(demolishButtonTooltip());
  _window->addChild(demolishButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::confirmAndDemolishObject(void *objectToDemolish) {
  assert(objectToDemolish != nullptr);
  ClientObject &obj = *reinterpret_cast<ClientObject *>(objectToDemolish);
  Client &client = *Client::_instance;
  std::string confirmationText = "Are you sure you want to " +
                                 obj.demolishVerb() + " this " + obj.name() +
                                 "? This cannot be undone.";
  if (obj._confirmDemolishWindow != nullptr)
    client.removeWindow(obj._confirmDemolishWindow);
  else
    obj._confirmDemolishWindow = new ConfirmationWindow(
        confirmationText, CL_DEMOLISH, makeArgs(obj.serial()));
  client.addWindow(obj._confirmDemolishWindow);
  obj._confirmDemolishWindow->show();
}

void ClientObject::assembleWindow(Client &client) {
  const ClientObjectType &objType = *objectType();

  static const px_t WINDOW_WIDTH = ContainerGrid(1, 8, _container).width();

  if (_window) {
    _window->clearChildren();
    _window->resize(0, 0);
  }

  if (lootable()) {
    static const px_t WIDTH = 100, HEIGHT = 100;
    _lootContainer =
        TakeContainer::CopyFrom(_container, serial(), {0, 0, WIDTH, HEIGHT});
    auto winRect = toScreenRect(location());
    winRect.w = WIDTH;
    winRect.h = HEIGHT;
    if (!_window)
      _window = Window::WithRectAndTitle(winRect, objectType()->name());
    else
      _window->resize(WIDTH, HEIGHT);
    _window->addChild(_lootContainer);

    return;
  }

  const bool userIsOwner = _owner == Owner{Owner::PLAYER, client.username()},
             hasContainer = objType.containerSlots() > 0 && userHasAccess(),
             isMerchant =
                 objType.merchantSlots() > 0 && userHasMerchantAccess(),
             canCede = (canBeOwnedByACity() &&
                        !client.character().cityName().empty()) &&
                       userIsOwner && (!objType.isPlayerUnique()),
             canGrant =
                 (client.character().isKing() &&
                  _owner == Owner{Owner::CITY, client.character().cityName()}),
             canDemolish = userHasDemolishAccess(),
             hasAQuest =
                 !(startsQuests().empty() && completableQuests().empty()) &&
                 userHasAccess();

  if (!_window) _window = Window::WithRectAndTitle({}, objType.name());

  auto windowHasClassContent = addClassSpecificStuffToWindow();
  auto hasNonDemolitionContent =
      windowHasClassContent || hasContainer || isMerchant || canCede ||
      canGrant || hasAQuest || objType.hasAction() || objType.canDeconstruct();
  auto hasAnyContent = hasNonDemolitionContent || canDemolish;

  if (isAlive()) {
    if (isBeingConstructed()) {
      if (userHasAccess()) {
        addConstructionToWindow();
        if (canCede) addCedeButtonToWindow();
        if (canGrant) addGrantButtonToWindow();
        if (canDemolish) addDemolishButtonToWindow();
        hasNonDemolitionContent = hasAnyContent = true;
      }

    } else if (userHasAccess()) {
      if (hasAQuest) addQuestsToWindow();
      if (isMerchant) addMerchantSetupToWindow();
      if (hasContainer) addInventoryToWindow();
      if (objType.hasAction()) addActionToWindow();
      if (objType.canDeconstruct()) addDeconstructionToWindow();
      if (canCede) addCedeButtonToWindow();
      if (canGrant) addGrantButtonToWindow();
      if (canDemolish) addDemolishButtonToWindow();

    } else if (userHasMerchantAccess()) {
      if (isMerchant && _owner.type != Owner::NO_ACCESS)
        addMerchantTradeToWindow();
    }
  }

  if (!hasAnyContent) {
    client.removeWindow(_window);
    delete _window;
    _window = nullptr;
    return;
  }

  px_t currentWidth = _window->contentWidth(),
       currentHeight = _window->contentHeight();
  _window->resize(currentWidth, currentHeight + BUTTON_GAP);

  if (!hasNonDemolitionContent) _window->hide();
}

void ClientObject::onInventoryUpdate() {
  if (_lootContainer) {
    _lootContainer->repopulate();
    if (_lootContainer->size() == 0) {
      _lootable = false;
      hideWindow();
    }

  } else if (_window)
    _window->forceRefresh();

  refreshTooltip();
}

void ClientObject::hideWindow() {
  if (_window != nullptr) _window->hide();
}

void ClientObject::startDeconstructing(void *object) {
  const ClientObject &obj = *static_cast<const ClientObject *>(object);
  Client &client = *Client::_instance;
  client.sendMessage({CL_DECONSTRUCT, obj.serial()});
  client.prepareAction(std::string("Dismantling ") + obj.objectType()->name());
}

void ClientObject::trade(Serial serial, size_t slot) {
  Client::_instance->sendMessage({CL_TRADE, makeArgs(serial, slot)});
}

void ClientObject::sendMerchantSlot(Serial serial, size_t slot) {
  const auto &objects = Client::_instance->_objects;
  auto it = objects.find(serial);
  if (it == objects.end()) {
    Client::instance().showErrorMessage(
        "Attempting to configure nonexistent object", Color::CHAT_ERROR);
    return;
  }
  ClientObject &obj = *it->second;
  ClientMerchantSlot &mSlot = obj._merchantSlots[slot];

  // Set quantities
  mSlot.wareQty = obj._wareQtyBoxes[slot]->textAsNum();
  mSlot.priceQty = obj._priceQtyBoxes[slot]->textAsNum();

  if (mSlot.wareItem == nullptr || mSlot.priceItem == nullptr) {
    Client::instance().showErrorMessage(
        "You must select an item; clearing slot.", Color::CHAT_WARNING);
    Client::_instance->sendMessage(
        {CL_CLEAR_MERCHANT_SLOT, makeArgs(serial, slot)});
    return;
  }

  Client::_instance->sendMessage(
      {CL_SET_MERCHANT_SLOT,
       makeArgs(serial, slot, mSlot.wareItem->id(), mSlot.wareQty,
                mSlot.priceItem->id(), mSlot.priceQty)});
}

bool ClientObject::userHasAccess() const {
  // No owner
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return true;
  if (_owner.type == Owner::NO_ACCESS) return false;

  // Player is owner
  if (isOwnedByPlayer(Client::_instance->username())) return true;

  // City is owner
  auto playerCity = Client::_instance->character().cityName();
  if (isOwnedByCity(playerCity)) return true;

  // Player-unique: fellow citizen is owner.  This is a special case where other
  // citizens get access.
  auto isPlayerUnique =
      dynamic_cast<const ClientObjectType *>(type())->isPlayerUnique();
  if (!isPlayerUnique || _owner.type != Owner::PLAYER) return false;
  auto ownerCity = Client::instance().getUserCity(_owner.name);
  auto isOwnedByFellowCitizen = playerCity == ownerCity && !playerCity.empty();
  return isOwnedByFellowCitizen;
}

bool ClientObject::userHasMerchantAccess() const {
  if (_owner.type == Owner::NO_ACCESS) return false;

  return true;
}

bool ClientObject::userHasDemolishAccess() const {
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return false;
  if (_owner.type == Owner::NO_ACCESS) return false;

  // Player is owner
  if (isOwnedByPlayer(Client::_instance->username())) return true;

  // City is owner
  auto playerCity = Client::_instance->character().cityName();
  if (isOwnedByCity(playerCity)) return true;

  return false;
}

bool ClientObject::canAlwaysSee() const {
  return isOwnedByPlayer(Client::_instance->username()) ||
         isOwnedByCity(Client::_instance->character().cityName());
}

void ClientObject::update(double delta) {
  if (this->isBeingConstructed()) {
    Sprite::update(delta);
    ClientCombatant::update(delta);
    return;
  }

  Client &client = *Client::_instance;
  ms_t timeElapsed = toInt(1000 * delta);

  // If being gathered, add particles and play sounds.
  if (beingGathered()) {
    client.addParticles(objectType()->gatherParticles(), location(), delta);
    if (_gatherSoundTimer > timeElapsed)
      _gatherSoundTimer -= timeElapsed;
    else {
      objectType()->playSoundOnce("gather");

      // Restart timer
      _gatherSoundTimer += objectType()->soundPeriod() - timeElapsed;
    }
  }

  // If transforming, reduce timer.
  if (isDead()) _transformTimer = 0;
  if (_transformTimer > 0) {
    if (timeElapsed > _transformTimer)
      _transformTimer = 0;
    else
      _transformTimer -= timeElapsed;
  }

  // If dead, add smoke particles
  if (isDead() && classTag() != 'n' && collisionRect().w != 0 &&
      collisionRect().h != 0) {
    auto particleLocation =
        MapPoint{randDouble() * collisionRect().w + collisionRect().x,
                 randDouble() * collisionRect().h + collisionRect().y};

    const auto *smokeProfile = client.findParticleProfile("smoke");
    if (smokeProfile) {
      const auto PARTICLES_PER_PIXEL = 0.005;
      auto area = collisionRect().w * collisionRect().h;
      auto numParticles = smokeProfile->numParticlesContinuous(
          delta * area * PARTICLES_PER_PIXEL);

      client.addParticles(smokeProfile, particleLocation, numParticles);
    }
  }

  // Loot sparkles
  if (lootable())
    Client::_instance->addParticles("lootSparkles", location(), delta);

  Sprite::update(delta);
}

void ClientObject::draw(const Client &client) const {
  Sprite::draw(client);
  drawAppropriateQuestIndicator();
}

void ClientObject::drawAppropriateQuestIndicator() const {
  static const auto questStartIndicator =
      Texture{"Images/questStart.png", Color::MAGENTA};
  static const auto questEndIndicator =
      Texture{"Images/questEnd.png", Color::MAGENTA};
  auto questIndicator = Texture{};

  if (!userHasAccess()) return;
  if (isBeingConstructed()) return;
  if (!completableQuests().empty())
    questIndicator = questEndIndicator;
  else if (!startsQuests().empty())
    questIndicator = questStartIndicator;
  else
    return;

  auto questIndicatorOffset = ScreenRect{
      -questIndicator.width() / 2, -questIndicator.height() - 17 - height(),
      questIndicator.width(), questIndicator.height()};
  auto indicatorLocation = toScreenRect(location()) +
                           Client::instance().offset() + questIndicatorOffset;
  questIndicator.draw(indicatorLocation);
}

const Texture &ClientObject::cursor(const Client &client) const {
  if (client.isAltPressed()) return client.cursorRepair();

  if (canBeAttackedByPlayer()) return client.cursorAttack();

  const ClientObjectType &ot = *objectType();
  if (userHasAccess()) {
    if (isBeingConstructed()) return client.cursorContainer();
    if (completableQuests().size() > 0) return client.cursorEndsQuest();
    if (startsQuests().size() > 0) return client.cursorStartsQuest();
    if (ot.canGather()) return client.cursorGather();
    if (ot.hasAction()) return client.cursorContainer();
    if (classTag() == 'v') return client.cursorVehicle();
    if (ot.containerSlots() > 0) return client.cursorContainer();
  }
  if (lootable() || ot.merchantSlots() > 0) return client.cursorContainer();

  return client.cursorNormal();
}

const Tooltip &ClientObject::tooltip() const {
  auto shouldShowRepairTooltip = Client::instance().isAltPressed();
  if (shouldShowRepairTooltip) {
    if (!_repairTooltip.hasValue()) createRepairTooltip();
    return _repairTooltip.value();
  }

  if (!_tooltip.hasValue()) createRegularTooltip();
  return _tooltip.value();
}

void ClientObject::createRegularTooltip() const {
  if (_tooltip.hasValue()) return;
  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  const auto &client = *Client::_instance;

  const ClientObjectType &ot = *objectType();

  auto isContainer = ot.containerSlots() > 0 && classTag() != 'n';
  auto startsAQuest = !startsQuests().empty();
  auto hasAction = ot.hasAction();

  // Name
  tooltip.setColor(Color::TOOLTIP_NAME);
  std::string title = ot.name();
  if (isDead())
    title += classTag() == 'n' ? " (corpse)" : " (ruins)";
  else if (isBeingConstructed()) {
    auto constructionText = ot.constructionText().empty()
                                ? "under construction"s
                                : ot.constructionText();
    title += " ("s + constructionText + ")"s;
  }
  tooltip.addLine(title);

  // Debug info
  if (isDebug()) {
    tooltip.addGap();
    tooltip.setColor(Color::DEBUG_TEXT);
    tooltip.addLine("Serial: " + toString(_serial));
    tooltip.addLine("Class tag: " + toString(classTag()));
  }

  // Level
  if (classTag() == 'n') {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_BODY);
    tooltip.addLine("Level "s + toString(level()));
  }

  // Owner
  tooltip.setColor(Color::TOOLTIP_BODY);
  if (isOwnedByPlayer(Client::_instance->username())) {
    tooltip.addGap();
    tooltip.addLine("Owned by you");
  } else if (_owner.type != Owner::ALL_HAVE_ACCESS) {
    tooltip.addGap();
    tooltip.addLine("Owned by " + _owner.name);
  }

  if (isDead()) return;

  if (isBeingConstructed()) {
    if (!userHasAccess()) return;

    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
    tooltip.addLine(std::string("Right-click to add materials"));
    return;
  }

  // Stats
  bool stats = false;
  tooltip.setColor(Color::TOOLTIP_BODY);

  if (addClassSpecificStuffToTooltip(tooltip)) {
    if (!stats) {
      stats = true;
      tooltip.addGap();
    }
  }

  if (userHasAccess()) {
    if (ot.canGather()) {
      if (!stats) {
        stats = true;
        tooltip.addGap();
      }
      std::string text = "Gatherable";
      if (!ot.gatherReq().empty())
        text += " (requires " + client.tagName(ot.gatherReq()) + ")";
      tooltip.addLine(text);
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine(std::string("Right-click to gather"));

      // Unlocks from gathering
      auto bestUnlockChance = 0.0;
      auto bestEffectInfo = Unlocks::EffectInfo{};
      for (auto &pair : objectType()->gatherChances()) {
        const auto &itemID = pair.first;
        auto gatherChance = pair.second;
        auto unlockInfo = Unlocks::getEffectInfo({Unlocks::GATHER, itemID});
        auto unlockChance = gatherChance * unlockInfo.chance;

        if (unlockChance > bestUnlockChance) {
          bestUnlockChance = unlockChance;
          bestEffectInfo = unlockInfo;
        }
      }
      if (bestEffectInfo.hasEffect) {
        tooltip.setColor(bestEffectInfo.color);
        tooltip.addLine(bestEffectInfo.message);
      }
    }

    if (ot.canDeconstruct()) {
      if (!stats) {
        stats = true;
        tooltip.addGap();
      }
      tooltip.addLine("Can pick up as item");
    }

    if (isContainer) {
      if (!stats) {
        stats = true;
        tooltip.addGap();
      }
      tooltip.addLine("Container: " + toString(ot.containerSlots()) + " slots");
      tooltip.addItemGrid(&_container);
    }
  }

  if (ot.merchantSlots() > 0) {
    if (!stats) {
      stats = true;
      tooltip.addGap();
    }
    tooltip.addLine("Merchant: " + toString(ot.merchantSlots()) + " slots");
    tooltip.addMerchantSlots(_merchantSlots);
  }

  // Tags
  tooltip.addTags(ot);

  // Any actions available?
  if (ot.merchantSlots() > 0 ||
      userHasAccess() && (classTag() == 'v' || isContainer ||
                          ot.canDeconstruct() || startsAQuest || hasAction)) {
    tooltip.addGap();
    tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
    tooltip.addLine(std::string("Right-click to interact"));
  }

  auto needsRepairing = health() < this->maxHealth();
  if (ot.repairInfo().canBeRepaired && needsRepairing) {
    tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
    tooltip.addLine("Alt-click to repair.");
  }

  // NPC-specific stuff
  else if (classTag() == 'n') {
    const ClientNPC &npc = dynamic_cast<const ClientNPC &>(*this);
    if (npc.canBeTamed()) {
      tooltip.addGap();
      tooltip.setColor(Color::TOOLTIP_BODY);
      tooltip.addLine("Can be tamed into a pet:");
      if (npc.npcType()->itemRequiredForTaming()) {
        tooltip.addLine("Will consume:");
        tooltip.addItem(*npc.npcType()->itemRequiredForTaming());
      }
      auto chance = toString(toInt(npc.getTameChance() * 100));
      tooltip.addLine(chance + "% chance to succeed");
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine(std::string("Ctrl-click to attempt"));
    }
    if (npc.canBeAttackedByPlayer()) {
      tooltip.addGap();
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine("Right-click to attack");
    } else if (npc.lootable()) {
      tooltip.addGap();
      tooltip.setColor(Color::TOOLTIP_INSTRUCTION);
      tooltip.addLine("Right-click to loot");
    }
  }
}

bool ClientObject::addClassSpecificStuffToTooltip(Tooltip &tooltip) const {
  return false;
}

void ClientObject::createRepairTooltip() const {
  const auto &client = *Client::_instance;
  const ClientObjectType &ot = *objectType();
  const auto &repairInfo = ot.repairInfo();

  if (!repairInfo.canBeRepaired) {
    _repairTooltip = Tooltip::basicTooltip("This object cannot be repaired.");
    return;
  }

  auto needsRepairing = health() < Item::MAX_HEALTH;
  if (!needsRepairing) {
    _repairTooltip =
        Tooltip::basicTooltip("This object is already at full health.");
    return;
  }

  _repairTooltip = Tooltip{};
  auto &rt = _repairTooltip.value();

  rt.setColor(Color::TOOLTIP_INSTRUCTION);
  rt.addLine("Alt-click to repair.");

  if (repairInfo.requiresTool()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Requires tool:");
    rt.setColor(Color::TOOLTIP_TAG);
    rt.addLine(Client::instance().tagName(repairInfo.tool));
  }

  if (repairInfo.hasCost()) {
    rt.addGap();
    rt.setColor(Color::TOOLTIP_BODY);
    rt.addLine("Will consume:");
    const auto *costItem = Client::instance().findItem(repairInfo.cost);
    if (!costItem) return;
    rt.addItem(*costItem);
  }
}

const Texture &ClientObject::image() const {
  if (health() == 0) return objectType()->corpseImage();
  if (isBeingConstructed())
    return objectType()->constructionImage().getNormalImage();
  if (objectType()->transforms())
    return objectType()->getProgressImage(_transformTimer).getNormalImage();
  return Sprite::image();
}

const Texture &ClientObject::getHighlightImage() const {
  if (health() == 0) return objectType()->corpseHighlightImage();
  if (isBeingConstructed())
    return objectType()->constructionImage().getHighlightImage();
  if (objectType()->transforms())
    return objectType()->getProgressImage(_transformTimer).getHighlightImage();
  return Sprite::getHighlightImage();
}

std::set<CQuest *> ClientObject::startsQuests() const {
  auto questsForThisType = objectType()->startsQuests();
  auto startableQuests = std::set<CQuest *>{};
  for (auto quest : questsForThisType) {
    if (quest->state == CQuest::CAN_START) startableQuests.insert(quest);
  }
  return startableQuests;
}

std::set<CQuest *> ClientObject::completableQuests() const {
  auto questsForThisType = objectType()->endsQuests();
  auto completableQuests = std::set<CQuest *>{};
  for (auto quest : questsForThisType) {
    if (quest->state == CQuest::CAN_FINISH) completableQuests.insert(quest);
  }
  return completableQuests;
}

void ClientObject::sendTargetMessage() const {
  const Client &client = *Client::_instance;
  client.sendMessage({CL_TARGET_ENTITY, serial()});
}

void ClientObject::sendSelectMessage() const {
  const Client &client = *Client::_instance;
  client.sendMessage({CL_SELECT_ENTITY, serial()});
}

bool ClientObject::canBeAttackedByPlayer() const {
  if (!ClientCombatant::canBeAttackedByPlayer()) return false;
  if (_owner.type == Owner::ALL_HAVE_ACCESS) return false;
  const Client &client = *Client::_instance;
  return client.isAtWarWithObjectOwner(_owner);
}

void ClientObject::playAttackSound() const {
  objectType()->playSoundOnce("attack");
}

void ClientObject::playDefendSound() const {
  objectType()->playSoundOnce("defend");
}

void ClientObject::playDeathSound() const {
  objectType()->playSoundOnce("death");
}

const Color &ClientObject::nameColor() const {
  if (belongsToPlayerCity()) return Color::COMBATANT_ALLY;

  if (belongsToPlayer()) return Color::COMBATANT_SELF;

  if (canBeAttackedByPlayer()) return Color::COMBATANT_DEFENSIVE;

  return Sprite::nameColor();
}

bool ClientObject::shouldDrawName() const {
  const auto &client = Client::instance();
  if (client.currentMouseOverEntity() == this) return true;
  return false;
}

bool ClientObject::shouldDrawShadow() const {
  if (isBeingConstructed()) return false;
  return Sprite::shouldDrawShadow();
}

bool ClientObject::containerIsEmpty() const {
  for (const auto &pair : _container)
    if (pair.first.type() != nullptr && pair.second > 0) return false;
  return true;
}

bool ClientObject::belongsToPlayer() const {
  const Avatar &playerCharacter = Client::_instance->character();
  return owner() == Owner{Owner::PLAYER, playerCharacter.name()};
}

bool ClientObject::belongsToPlayerCity() const {
  const Avatar &playerCharacter = Client::_instance->character();
  if (playerCharacter.cityName().empty()) return false;
  return owner() == Owner{Owner::CITY, playerCharacter.cityName()};
}

std::string ClientObject::additionalTextInName() const {
  // Transform time
  if (_transformTimer > 0 && userHasAccess() && !isBeingConstructed())
    return "("s + msAsTimeDisplay(_transformTimer) + " remaining)"s;
  return {};
}

bool ClientObject::isFlat() const { return Sprite::isFlat() || isDead(); }

ClientObject::Owner::Owner(Type typeArg, std::string nameArg)
    : type(typeArg), name(nameArg) {}

bool ClientObject::Owner::operator==(Owner &rhs) const {
  if (type != rhs.type) return false;
  if (type == PLAYER || type == CITY) return name == rhs.name;
  return true;
}
