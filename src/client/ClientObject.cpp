#include <SDL_mixer.h>
#include <cassert>

#include "../Color.h"
#include "../Log.h"
#include "../util.h"
#include "Client.h"
#include "ClientNPC.h"
#include "ClientObject.h"
#include "ClientVehicle.h"
#include "Particle.h"
#include "Renderer.h"
#include "Tooltip.h"
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
      _container(rhs._container),
      _window(nullptr),
      _confirmCedeWindow(nullptr),
      _beingGathered(rhs._beingGathered) {}

ClientObject::ClientObject(size_t serialArg, const ClientObjectType *type,
                           const MapPoint &loc)
    : Sprite(type, loc),
      ClientCombatant(type),
      _serial(serialArg),
      _window(nullptr),
      _confirmCedeWindow(nullptr),
      _beingGathered(false),
      _dropbox(1),
      _transformTimer(type->transformTime()),
      _gatherSoundTimer(0),
      _lootable(false),
      _lootContainer(nullptr) {
  if (type == nullptr)  // i.e., a serial-only search dummy
    return;

  const size_t containerSlots = objectType()->containerSlots(),
               merchantSlots = objectType()->merchantSlots();
  _container = ClientItem::vect_t(containerSlots);
  _merchantSlots = std::vector<ClientMerchantSlot>(merchantSlots);
  _merchantSlotElements = std::vector<Element *>(merchantSlots, nullptr);
  _serialSlotPairs = std::vector<serialSlotPair_t *>(merchantSlots, nullptr);
  for (size_t i = 0; i != merchantSlots; ++i) {
    serialSlotPair_t *pair = new serialSlotPair_t();
    pair->first = _serial;
    pair->second = i;
    _serialSlotPairs[i] = pair;
  }
  _wareQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
  _priceQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
}

ClientObject::~ClientObject() {
  for (auto p : _serialSlotPairs) delete p;
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

void ClientObject::setMerchantSlot(size_t i, ClientMerchantSlot &mSlotArg) {
  _merchantSlots[i] = mSlotArg;
  ClientMerchantSlot &mSlot = _merchantSlots[i];

  if (_window == nullptr || isBeingConstructed()) return;
  assert(_merchantSlotElements[i] != nullptr);

  // Update slot element
  Element &e = *_merchantSlotElements[i];
  const ClientItem *wareItem = toClientItem(mSlot.wareItem),
                   *priceItem = toClientItem(mSlot.priceItem);
  e.clearChildren();

  static const px_t  // TODO: remove duplicate consts
      NAME_WIDTH = 100,
      QUANTITY_WIDTH = 20, BUTTON_PADDING = 1,
      TEXT_HEIGHT = Element::TEXT_HEIGHT, ICON_SIZE = Element::ITEM_HEIGHT,
      BUTTON_LABEL_WIDTH = 45, BUTTON_HEIGHT = ICON_SIZE + 2 * BUTTON_PADDING,
      BUTTON_TOP = GAP + 2,
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
                   "Set", sendMerchantSlot, _serialSlotPairs[i]));

  } else {  // Trade view
    if (!mSlot) {
      return;  // Blank
    }

    // Ware
    px_t x = GAP;
    e.addChild(new Label({x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT},
                         toString(mSlot.wareQty), Element::RIGHT_JUSTIFIED));
    x += QUANTITY_WIDTH;
    e.addChild(
        new Picture({x, (ROW_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE},
                    wareItem->icon()));
    x += ICON_SIZE;
    e.addChild(
        new Label({x, TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT}, wareItem->name()));
    x += NAME_WIDTH + GAP;

    // Buy button
    Button *button = new Button({x, GAP, BUTTON_WIDTH, BUTTON_HEIGHT}, "",
                                trade, _serialSlotPairs[i]);
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
}

void ClientObject::onLeftClick(Client &client) {
  client.setTarget(*this);

  // Note: parent class's onLeftClick() not called.
}

void ClientObject::onRightClick(Client &client) {
  client.setTarget(*this, canBeAttackedByPlayer());

  // Make sure object is in range
  if (distance(client.playerCollisionRect(), collisionRect()) >
      Client::ACTION_DISTANCE) {
    client.showErrorMessage("That object is too far away.", Color::WARNING);
    return;
  }

  // Gatherable
  const ClientObjectType &objType = *objectType();
  if (objType.canGather() && userHasAccess()) {
    client.sendMessage(CL_GATHER, makeArgs(_serial));
    client.prepareAction(std::string("Gathering ") + objType.name());
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
    if (_window != nullptr) client.addWindow(_window);
  }

  // Watch object
  if (objType.containerSlots() > 0 || objType.merchantSlots() > 0 ||
      isBeingConstructed() || _lootable)
    client.watchObject(*this);

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

  const auto BUTTON_HEIGHT = 15, BUTTON_WIDTH = 100, MARGIN = 2;

  _window->addChild(new Label({MARGIN, y, BUTTON_WIDTH, Element::TEXT_HEIGHT},
                              "New quests:"));
  y += Element::TEXT_HEIGHT + MARGIN;

  static auto newQuestIcon = Texture{"Images/UI/newQuest.png", Color::MAGENTA};
  const auto BUTTON_X = newQuestIcon.width();

  for (const auto &quest : startsQuests()) {
    y += MARGIN;

    _window->addChild(new Picture(
        0, y + (BUTTON_HEIGHT - newQuestIcon.height()) / 2, newQuestIcon));

    const auto buttonRect =
        ScreenRect{BUTTON_X, y, BUTTON_WIDTH, BUTTON_HEIGHT};
    auto data = makeArgs(quest->id(), serial());
    auto button =
        new Button(buttonRect, quest->name(),
                   Client::sendMessageWithString<CL_ACCEPT_QUEST>, data);
    button->id(quest->id());
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
  Button *deconstructButton = new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT},
                                         "Pick up", startDeconstructing, this);
  _window->addChild(deconstructButton);
  y += BUTTON_GAP + BUTTON_HEIGHT;
  x += BUTTON_GAP + BUTTON_WIDTH;
  if (newWidth < x) newWidth = x;

  _window->resize(newWidth, y);
}

void ClientObject::addVehicleToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *mountButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Enter/exit",
                 ClientVehicle::mountOrDismount, this);
  _window->addChild(mountButton);
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

  // Text input
  if (!action.textInput.empty()) {
    const auto LABEL_WIDTH = px_t{50};
    auto *label = new Label({x, y, LABEL_WIDTH, 13}, action.textInput);
    _window->addChild(label);
    x += LABEL_WIDTH + BUTTON_GAP;

    _actionTextEntry = new TextBox({x, y, 50, 13}, TextBox::LETTERS);
    _window->addChild(_actionTextEntry);
    y += 13 + BUTTON_GAP;
    x += 50;
    if (newWidth < x) newWidth = x;
  }

  // Button
  x = BUTTON_GAP;
  Button *button = new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, action.label,
                              performAction, this);

  auto tooltipNeeded = !action.tooltip.empty() || action.cost;
  if (tooltipNeeded) {
    Tooltip tooltip;
    if (!action.tooltip.empty()) tooltip.addLine(action.tooltip);

    if (!action.tooltip.empty() && action.cost) tooltip.addGap();

    if (action.cost) {
      tooltip.setColor(Color::ITEM_TAGS);
      tooltip.addLine("Consumes "s + action.cost->name());
    }

    button->setTooltip(tooltip);
  }
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

  client.sendMessage(CL_PERFORM_OBJECT_ACTION, makeArgs(obj.serial(), textArg));
}

void ClientObject::addMerchantTradeToWindow() {
  px_t y = _window->contentHeight(), newWidth = _window->contentWidth();

  static const px_t NAME_WIDTH = 100, QUANTITY_WIDTH = 20, BUTTON_PADDING = 1,
                    BUTTON_LABEL_WIDTH = 45,
                    BUTTON_HEIGHT = Element::ITEM_HEIGHT + 2 * BUTTON_PADDING,
                    ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
                    WIDTH = 2 * Element::ITEM_HEIGHT + 2 * NAME_WIDTH +
                            QUANTITY_WIDTH + BUTTON_LABEL_WIDTH +
                            2 * BUTTON_PADDING + 3 * GAP + List::ARROW_W;
  const double MAX_ROWS = 7.5,
               NUM_ROWS = objectType()->merchantSlots() < MAX_ROWS
                              ? objectType()->merchantSlots()
                              : MAX_ROWS;
  static const px_t HEIGHT = toInt(ROW_HEIGHT * NUM_ROWS);
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
  Button *cedeButton = new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT},
                                  "Cede to city", confirmAndCedeObject, this);
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
                 getInputAndGrantObject, this);
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
    obj._grantWindow =
        new InputWindow(windowText, CL_GRANT, makeArgs(obj.serial()));
  client.addWindow(obj._grantWindow);
  obj._grantWindow->show();
}

void ClientObject::addDemolishButtonToWindow() {
  px_t x = BUTTON_GAP, y = _window->contentHeight(),
       newWidth = _window->contentWidth();
  y += BUTTON_GAP;
  Button *demolishButton =
      new Button({x, y, BUTTON_WIDTH, BUTTON_HEIGHT}, "Demolish",
                 confirmAndDemolishObject, this);
  demolishButton->setTooltip("Demolish this object, removing it permanently");
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
  std::string confirmationText = "Are you sure you want to demolish this " +
                                 obj.name() + "? This cannot be undone.";
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

  if (_window != nullptr) {
    _window->clearChildren();
    _window->resize(0, 0);
  }

  if (lootable()) {
    static const px_t WIDTH = 100, HEIGHT = 100;
    _lootContainer =
        new TakeContainer(container(), serial(), {0, 0, WIDTH, HEIGHT});
    auto winRect = toScreenRect(location());
    winRect.w = WIDTH;
    winRect.h = HEIGHT;
    if (_window == nullptr)
      _window = Window::WithRectAndTitle(winRect, objectType()->name());
    else
      _window->resize(WIDTH, HEIGHT);
    _window->addChild(_lootContainer);

    return;
  }

  bool hasContainer = objType.containerSlots() > 0,
       isMerchant = objType.merchantSlots() > 0, isVehicle = classTag() == 'v',
       canCede = (!client.character().cityName().empty()) &&
                 (_owner == client.username()) && (!objType.isPlayerUnique()),
       canGrant = (client.character().isKing() &&
                   _owner == client.character().cityName()),
       canDemolish = _owner == Client::_instance->username(),
       startsAQuest = !startsQuests().empty();
  if (isAlive() && (isMerchant ||
                    userHasAccess() &&
                        (hasContainer || isVehicle || objType.hasAction() ||
                         objType.canDeconstruct() || isBeingConstructed() ||
                         canCede || canGrant || canDemolish || startsAQuest))) {
    if (_window == nullptr)
      _window = Window::WithRectAndTitle({}, objType.name());

    if (isBeingConstructed()) {
      if (userHasAccess()) {
        client.watchObject(*this);
        addConstructionToWindow();
        if (canCede) addCedeButtonToWindow();
        if (canDemolish) addDemolishButtonToWindow();
      }

    } else if (!userHasAccess()) {
      if (isMerchant) {
        addMerchantTradeToWindow();
      }

    } else {
      if (startsAQuest) addQuestsToWindow();
      if (isMerchant) addMerchantSetupToWindow();
      if (hasContainer) {
        client.watchObject(*this);
        addInventoryToWindow();
      }
      if (isVehicle) addVehicleToWindow();
      if (objType.hasAction()) addActionToWindow();
      if (objType.canDeconstruct()) addDeconstructionToWindow();
      if (canCede) addCedeButtonToWindow();
      if (canGrant) addGrantButtonToWindow();
      if (canDemolish) addDemolishButtonToWindow();
    }

    px_t currentWidth = _window->contentWidth(),
         currentHeight = _window->contentHeight();
    _window->resize(currentWidth, currentHeight + BUTTON_GAP);

    return;
  }

  if (_window != nullptr) {
    _window->hide();
    client.removeWindow(_window);
    delete _window;
    _window = nullptr;
  }
}

void ClientObject::startsQuest(const std::string questID) {
  const Client &client = Client::instance();
  auto it = client.quests().find(questID);
  if (it == client.quests().end()) return;
  const auto &quest = it->second;
  _questsStartingHere.insert(&quest);
}

void ClientObject::onInventoryUpdate() {
  if (_lootContainer != nullptr) _lootContainer->repopulate();
  if (_window != nullptr) _window->forceRefresh();
}

void ClientObject::hideWindow() {
  if (_window != nullptr) _window->hide();
}

void ClientObject::startDeconstructing(void *object) {
  const ClientObject &obj = *static_cast<const ClientObject *>(object);
  Client &client = *Client::_instance;
  client.sendMessage(CL_DECONSTRUCT, makeArgs(obj.serial()));
  client.prepareAction(std::string("Dismantling ") + obj.objectType()->name());
}

void ClientObject::trade(void *serialAndSlot) {
  const std::pair<size_t, size_t> pair =
      *static_cast<std::pair<size_t, size_t> *>(serialAndSlot);
  Client::_instance->sendMessage(CL_TRADE, makeArgs(pair.first, pair.second));
}

void ClientObject::sendMerchantSlot(void *serialAndSlot) {
  const std::pair<size_t, size_t> pair =
      *static_cast<std::pair<size_t, size_t> *>(serialAndSlot);
  size_t serial = pair.first, slot = pair.second;
  const auto &objects = Client::_instance->_objects;
  auto it = objects.find(serial);
  if (it == objects.end()) {
    Client::instance().showErrorMessage(
        "Attempting to configure nonexistent object", Color::FAILURE);
    return;
  }
  ClientObject &obj = *it->second;
  ClientMerchantSlot &mSlot = obj._merchantSlots[slot];

  // Set quantities
  mSlot.wareQty = obj._wareQtyBoxes[slot]->textAsNum();
  mSlot.priceQty = obj._priceQtyBoxes[slot]->textAsNum();

  if (mSlot.wareItem == nullptr || mSlot.priceItem == nullptr) {
    Client::instance().showErrorMessage(
        "You must select an item; clearing slot.", Color::WARNING);
    Client::_instance->sendMessage(CL_CLEAR_MERCHANT_SLOT,
                                   makeArgs(serial, slot));
    return;
  }

  Client::_instance->sendMessage(
      CL_SET_MERCHANT_SLOT,
      makeArgs(serial, slot, mSlot.wareItem->id(), mSlot.wareQty,
               mSlot.priceItem->id(), mSlot.priceQty));
}

bool ClientObject::userHasAccess() const {
  return _owner.empty() || _owner == Client::_instance->username() ||
         _owner == Client::_instance->character().cityName();
}

bool ClientObject::canAlwaysSee() const {
  return _owner == Client::_instance->username() ||
         _owner == Client::_instance->character().cityName();
}

void ClientObject::update(double delta) {
  if (this->isBeingConstructed()) {
    Sprite::update(delta);
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
      const SoundProfile *sounds = objectType()->sounds();
      if (sounds != nullptr) {
        // Play sound
        sounds->playOnce("gather");

        // Restart timer
        _gatherSoundTimer += objectType()->sounds()->period() - timeElapsed;
      }
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

  if (!startsQuests().empty()) {
    static const auto questStartIndicator =
        Texture{"Images/questStart.png", Color::MAGENTA};
    auto questStartIndicatorOffset =
        ScreenRect{-questStartIndicator.width() / 2,
                   -questStartIndicator.height() - 7 - height(),
                   questStartIndicator.width(), questStartIndicator.height()};
    auto indicatorLocation =
        toScreenRect(location()) + client.offset() + questStartIndicatorOffset;
    questStartIndicator.draw(indicatorLocation);
  }

  drawHealthBarIfAppropriate(location(), height());
}

const Texture &ClientObject::cursor(const Client &client) const {
  if (canBeAttackedByPlayer()) return client.cursorAttack();

  const ClientObjectType &ot = *objectType();
  if (userHasAccess()) {
    if (startsQuests().size() > 0) return client.cursorQuest();
    if (ot.canGather()) return client.cursorGather();
    if (ot.containerSlots() > 0) return client.cursorContainer();
  }
  if (lootable() || ot.merchantSlots() > 0) return client.cursorContainer();

  return client.cursorNormal();
}

const Tooltip &ClientObject::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();
  _tooltip = Tooltip{};
  auto &tooltip = _tooltip.value();

  const auto &client = *Client::_instance;

  const ClientObjectType &ot = *objectType();

  auto isContainer = ot.containerSlots() > 0 && classTag() != 'n';
  auto startsAQuest = !startsQuests().empty();

  // Name
  tooltip.setColor(Color::ITEM_NAME);
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
    tooltip.setColor(Color::ITEM_TAGS);
    tooltip.addLine("Serial: " + toString(_serial));
    tooltip.addLine("Class tag: " + toString(classTag()));
  }

  // Level
  if (classTag() == 'n') {
    tooltip.addGap();
    tooltip.setColor(Color::ITEM_TAGS);
    tooltip.addLine("Level "s + toString(level()));
  }

  // Owner
  if (!owner().empty()) {
    tooltip.addGap();
    tooltip.setColor(Color::ITEM_TAGS);
    tooltip.addLine("Owned by " + (owner() == Client::_instance->username()
                                       ? "you"
                                       : owner()));
  }

  if (isDead()) return tooltip;
  if (isBeingConstructed()) return tooltip;

  // Stats
  bool stats = false;
  tooltip.setColor(Color::ITEM_STATS);

  if (classTag() == 'v') {
    if (!stats) {
      stats = true;
      tooltip.addGap();
    }
    tooltip.addLine("Vehicle");
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
    }
  }

  if (ot.merchantSlots() > 0) {
    if (!stats) {
      stats = true;
      tooltip.addGap();
    }
    tooltip.addLine("Merchant: " + toString(ot.merchantSlots()) + " slots");
  }

  // Tags
  if (ot.hasTags()) {
    tooltip.addGap();
    tooltip.setColor(Color::ITEM_TAGS);
    for (const std::string &tag : ot.tags())
      tooltip.addLine(client.tagName(tag));
  }

  // Any actions available?
  if (ot.merchantSlots() > 0 ||
      userHasAccess() && (classTag() == 'v' || isContainer ||
                          ot.canDeconstruct() || startsAQuest)) {
    tooltip.addGap();
    tooltip.setColor(Color::ITEM_INSTRUCTIONS);
    tooltip.addLine(std::string("Right-click to interact"));
  }

  else if (classTag() == 'n') {
    tooltip.setColor(Color::ITEM_INSTRUCTIONS);
    const ClientNPC &npc = dynamic_cast<const ClientNPC &>(*this);
    if (npc.canBeAttackedByPlayer()) {
      tooltip.addGap();
      tooltip.addLine("Right-click to attack");
    } else if (npc.lootable()) {
      tooltip.addGap();
      tooltip.addLine("Right-click to loot");
    }
  }

  else if (userHasAccess() && ot.canGather()) {
    tooltip.addGap();
    tooltip.setColor(Color::ITEM_INSTRUCTIONS);
    tooltip.addLine(std::string("Right-click to gather"));
  }

  return tooltip;
}

const Texture &ClientObject::image() const {
  if (health() == 0) return objectType()->corpseImage();
  if (isBeingConstructed()) return objectType()->constructionImage().normal;
  if (objectType()->transforms())
    return objectType()->getProgressImage(_transformTimer).normal;
  return Sprite::image();
}

const Texture &ClientObject::highlightImage() const {
  if (health() == 0) return objectType()->corpseHighlightImage();
  if (isBeingConstructed()) return objectType()->constructionImage().highlight;
  if (objectType()->transforms())
    return objectType()->getProgressImage(_transformTimer).highlight;
  return Sprite::highlightImage();
}

void ClientObject::sendTargetMessage() const {
  const Client &client = *Client::_instance;
  client.sendMessage(CL_TARGET_ENTITY, makeArgs(serial()));
}

void ClientObject::sendSelectMessage() const {
  const Client &client = *Client::_instance;
  client.sendMessage(CL_SELECT_ENTITY, makeArgs(serial()));
}

bool ClientObject::canBeAttackedByPlayer() const {
  if (!ClientCombatant::canBeAttackedByPlayer()) return false;
  if (_owner.empty()) return false;
  const Client &client = *Client::_instance;
  return client.isAtWarWith(_owner);
}

void ClientObject::playAttackSound() const {
  auto sounds = objectType()->sounds();
  if (sounds) sounds->playOnce("attack");
}

void ClientObject::playDefendSound() const {
  auto sounds = objectType()->sounds();
  if (sounds) sounds->playOnce("defend");
}

void ClientObject::playDeathSound() const {
  auto sounds = objectType()->sounds();
  if (sounds) sounds->playOnce("death");
}

const Color &ClientObject::nameColor() const {
  if (belongsToPlayerCity()) return Color::COMBATANT_ALLY;

  if (belongsToPlayer()) return Color::COMBATANT_SELF;

  if (canBeAttackedByPlayer()) return Color::COMBATANT_ENEMY;

  return Sprite::nameColor();
}

bool ClientObject::shouldDrawName() const {
  const auto &client = Client::instance();
  if (client.currentMouseOverEntity() == this) return true;
  return false;
}

bool ClientObject::belongsToPlayer() const {
  const Avatar &playerCharacter = Client::_instance->character();
  return owner() == playerCharacter.name();
}

bool ClientObject::belongsToPlayerCity() const {
  const Avatar &playerCharacter = Client::_instance->character();
  if (playerCharacter.cityName().empty()) return false;
  return owner() == playerCharacter.cityName();
}

std::string ClientObject::additionalTextInName() const {
  if (_transformTimer > 0 && !this->isBeingConstructed())
    return "("s + msAsTimeDisplay(_transformTimer) + " remaining)"s;
  return {};
}

bool ClientObject::isFlat() const { return Sprite::isFlat() || isDead(); }
