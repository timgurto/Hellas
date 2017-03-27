#include <SDL_mixer.h>
#include <cassert>

#include "ClientNPC.h"
#include "ClientObject.h"
#include "ClientVehicle.h"
#include "Client.h"
#include "Particle.h"
#include "Renderer.h"
#include "TooltipBuilder.h"
#include "ui/Button.h"
#include "ui/Container.h"
#include "ui/ItemSelector.h"
#include "ui/Label.h"
#include "ui/Line.h"
#include "ui/TextBox.h"
#include "ui/Window.h"
#include "../Color.h"
#include "../Log.h"
#include "../util.h"

extern Renderer renderer;

const px_t ClientObject::BUTTON_HEIGHT = 15;
const px_t ClientObject::BUTTON_WIDTH = 60;
const px_t ClientObject::GAP = 2;
const px_t ClientObject::BUTTON_GAP = 1;

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial),
_container(rhs._container),
_window(nullptr),
_beingGathered(rhs._beingGathered){}

ClientObject::ClientObject(size_t serialArg, const ClientObjectType *type, const Point &loc):
Entity(type, loc),
_serial(serialArg),
_window(nullptr),
_beingGathered(false),
_dropbox(1),
_transformTimer(type->transformTime())
{
    if (type != nullptr) { // i.e., not a serial-only search dummy
        const size_t
            containerSlots = objectType()->containerSlots(),
            merchantSlots = objectType()->merchantSlots();
        _container = ClientItem::vect_t(containerSlots);
        _merchantSlots = std::vector<ClientMerchantSlot>(merchantSlots);
        _merchantSlotElements = std::vector<Element *>(merchantSlots, nullptr);
        _serialSlotPairs = std::vector<serialSlotPair_t *>(merchantSlots, nullptr);
        for (size_t i = 0; i != merchantSlots; ++i){
            serialSlotPair_t *pair = new serialSlotPair_t();
            pair->first = _serial;
            pair->second = i;
            _serialSlotPairs[i] = pair;
        }
        _wareQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
        _priceQtyBoxes = std::vector<TextBox *>(merchantSlots, nullptr);
    }
}

ClientObject::~ClientObject(){
    for (auto p : _serialSlotPairs)
        delete p;
    if (_window != nullptr) {
        Client::_instance->removeWindow(_window);
        delete _window;
    }
}

void ClientObject::setMerchantSlot(size_t i, ClientMerchantSlot &mSlotArg){
    _merchantSlots[i] = mSlotArg;
    ClientMerchantSlot &mSlot = _merchantSlots[i];

    if (_window == nullptr || isBeingConstructed())
        return;
    assert(_merchantSlotElements[i] != nullptr);

    // Update slot element
    Element &e = *_merchantSlotElements[i];
    const ClientItem
        *wareItem = toClientItem(mSlot.wareItem),
        *priceItem = toClientItem(mSlot.priceItem);
    e.clearChildren();

    static const px_t // TODO: remove duplicate consts
        NAME_WIDTH = 100,
        QUANTITY_WIDTH = 20,
        BUTTON_PADDING = 1,
        TEXT_HEIGHT = Element::TEXT_HEIGHT,
        ICON_SIZE = Element::ITEM_HEIGHT,
        BUTTON_LABEL_WIDTH = 45,
        BUTTON_HEIGHT = ICON_SIZE + 2 * BUTTON_PADDING,
        BUTTON_TOP = GAP + 2,
        BUTTON_WIDTH = BUTTON_PADDING * 2 + BUTTON_LABEL_WIDTH + ICON_SIZE + NAME_WIDTH,
        ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
        TEXT_TOP = (ROW_HEIGHT - TEXT_HEIGHT) / 2,
        BUTTON_TEXT_TOP = (BUTTON_HEIGHT - TEXT_HEIGHT) / 2,
        SET_BUTTON_WIDTH = 60,
        SET_BUTTON_HEIGHT = 15,
        SET_BUTTON_TOP = (ROW_HEIGHT - SET_BUTTON_HEIGHT) / 2;

    if (userHasAccess()){ // Setup view
        px_t x = GAP;
        TextBox *textBox = new TextBox(Rect(x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT), true);
        _wareQtyBoxes[i] = textBox;
        textBox->text(toString(mSlot.wareQty));
        e.addChild(textBox);
        x += QUANTITY_WIDTH + GAP;
        e.addChild(new ItemSelector(mSlot.wareItem, x, BUTTON_TOP));
        x += ICON_SIZE + 2 + NAME_WIDTH + 3 * GAP + 2;
        textBox = new TextBox(Rect(x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT), true);
        _priceQtyBoxes[i] = textBox;
        textBox->text(toString(mSlot.priceQty));
        e.addChild(textBox);
        x += QUANTITY_WIDTH + GAP;
        e.addChild(new ItemSelector(mSlot.priceItem, x, BUTTON_TOP));
        x += ICON_SIZE + 2 + NAME_WIDTH + 2 * GAP + 2;
        e.addChild(new Button(Rect(x, SET_BUTTON_TOP, SET_BUTTON_WIDTH, SET_BUTTON_HEIGHT), "Set",
                              sendMerchantSlot, _serialSlotPairs[i]));

    } else { // Trade view
        if (!mSlot){
            return; // Blank
        }

        // Ware
        px_t x = GAP;
        e.addChild(new Label(Rect(x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT),
                             toString(mSlot.wareQty), Element::RIGHT_JUSTIFIED));
        x += QUANTITY_WIDTH;
        e.addChild(new Picture(Rect(x, (ROW_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE),
                               wareItem->icon()));
        x += ICON_SIZE;
        e.addChild(new Label(Rect(x, TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT), wareItem->name()));
        x += NAME_WIDTH + GAP;
    
        // Buy button
        Button *button = new Button(Rect(x, GAP, BUTTON_WIDTH, BUTTON_HEIGHT), "", trade,
                                    _serialSlotPairs[i]);
        e.addChild(button);
        x = BUTTON_PADDING;
        button->addChild(new Label(Rect(x, BUTTON_TEXT_TOP, BUTTON_LABEL_WIDTH, TEXT_HEIGHT),
                                   std::string("Buy for ") + toString(mSlot.priceQty),
                                   Element::RIGHT_JUSTIFIED));
        x += BUTTON_LABEL_WIDTH;
        button->addChild(new Picture(Rect(x, BUTTON_PADDING, ICON_SIZE, ICON_SIZE),
                                     priceItem->icon()));
        x += ICON_SIZE;
        button->addChild(new Label(Rect(x, BUTTON_TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT),
                                   priceItem->name()));
    }
}

void ClientObject::onRightClick(Client &client){
    //Client::debug() << "Right-clicked on object #" << _serial << Log::endl;

    const ClientObjectType &objType = *objectType();

    // Make sure object is in range
    if (distance(client.playerCollisionRect(), collisionRect()) > Client::ACTION_DISTANCE) {
        client._debug("That object is too far away.", Color::WARNING);
        return;
    }
    
    // Gatherable
    if (objType.canGather() && userHasAccess()) {
        client.sendMessage(CL_GATHER, makeArgs(_serial));
        client.prepareAction(std::string("Gathering ") + objType.name());
        playGatherSound();
        return;
    }
    
    // Bring existing window to front
    if (_window != nullptr){
        client.removeWindow(_window);
        client.addWindow(_window);
    }

    // Create window, if necessary
    if (_window == nullptr){
        assembleWindow(client);
        if (_window != nullptr)
            client.addWindow(_window);
    }

    // Watch object
    if (objType.containerSlots() > 0 || objType.merchantSlots() > 0 || isBeingConstructed())
        client.watchObject(*this);

    if (_window != nullptr) {
        // Determine placement: below object, but keep entirely on screen.
        px_t x = toInt(location().x - _window->width() / 2 + client.offset().x);
        x = max(0, min(x, Client::SCREEN_X - _window->width()));
        static const px_t WINDOW_GAP_FROM_OBJECT = 20;
        px_t y = toInt(location().y + WINDOW_GAP_FROM_OBJECT / 2 + client.offset().y);
        y = max(0, min(y, Client::SCREEN_Y - _window->height()));
        _window->rect(x, y);

        _window->show();
    }
}

void ClientObject::addConstructionToWindow(){
    px_t
        x = 0,
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();
    static const px_t LABEL_W = 140;

    _window->addChild(new Label(Rect(x, y, LABEL_W, Element::TEXT_HEIGHT),
                                "Under construction"));
    if (newWidth < LABEL_W)
        newWidth = LABEL_W;
    y += Element::TEXT_HEIGHT + GAP;

    // 1. Required materials
    _window->addChild(new Label(Rect(x, y, LABEL_W, Element::TEXT_HEIGHT),
                                "Remaining materials required:"));
    y += Element::TEXT_HEIGHT;
    for (const auto &pair : constructionMaterials()){
        // Quantity
        static const px_t
            QTY_WIDTH = 20;
        _window->addChild(new Label(Rect(x, y, QTY_WIDTH, Client::ICON_SIZE),
                                    makeArgs(pair.second),
                                    Element::RIGHT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        x += QTY_WIDTH + GAP;
        // Icon
        const ClientItem &item = *dynamic_cast<const ClientItem *>(pair.first);
        _window->addChild(new Picture(x, y, item.icon()));
        x += Client::ICON_SIZE + GAP;
        // Name
        _window->addChild(new Label(Rect(x, y, LABEL_W, Client::ICON_SIZE),
                                    item.name(),
                                    Element::LEFT_JUSTIFIED, Element::CENTER_JUSTIFIED));
        y += Client::ICON_SIZE + GAP;
        if (newWidth < x)
            newWidth = x;
        x = BUTTON_GAP;
    }

    // 2. Dropbox
    static const px_t
        DROPBOX_LABEL_W = 70;
    Container *dropbox = new Container(1, 1, _dropbox, _serial, x, y);
    _window->addChild(new Label(Rect(x, y, DROPBOX_LABEL_W, dropbox->height()),
                                "Add materials:",
                                Element::RIGHT_JUSTIFIED, Element::CENTER_JUSTIFIED));
    x += DROPBOX_LABEL_W + GAP;
    dropbox->rect(x, y);
    _window->addChild(dropbox);
    y += dropbox->height() + GAP;

    _window->resize(newWidth, y);
}

void ClientObject::addMerchantSetupToWindow(){
    px_t
        x = 0,
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();

        static const px_t
            QUANTITY_WIDTH = 20,
            NAME_WIDTH = 100,
            PANE_WIDTH = Element::ITEM_HEIGHT + QUANTITY_WIDTH + NAME_WIDTH + 2 * GAP,
            TITLE_HEIGHT = 14,
            SET_BUTTON_WIDTH = 60,
            ROW_HEIGHT = Element::ITEM_HEIGHT + 4 * GAP;
        static const double
            MAX_ROWS = 5.5;
        const px_t
            LIST_HEIGHT = toInt(ROW_HEIGHT * min(MAX_ROWS, objectType()->merchantSlots()));
        x = GAP;
        _window->addChild(new Label(Rect(x, y, PANE_WIDTH, TITLE_HEIGHT), "Item to sell",
                                    Element::CENTER_JUSTIFIED));
        x += PANE_WIDTH + GAP;
        Line *vertDivider = new Line(x, y, TITLE_HEIGHT + LIST_HEIGHT, Line::VERTICAL);
        _window->addChild(vertDivider);
        x += vertDivider->width() + GAP;
        _window->addChild(new Label(Rect(x, y, PANE_WIDTH, TITLE_HEIGHT), "Price",
                                    Element::CENTER_JUSTIFIED));
        x += PANE_WIDTH + GAP;
        vertDivider = new Line(x, y, TITLE_HEIGHT + LIST_HEIGHT, Line::VERTICAL);
        _window->addChild(vertDivider);
        x += 2 * GAP + vertDivider->width() + SET_BUTTON_WIDTH;
        x += List::ARROW_W;
        if (newWidth < x)
            newWidth = x;
        y += Element::TEXT_HEIGHT;
        List *merchantList = new List(Rect(0, y,
                                           PANE_WIDTH * 2 + GAP * 5 + SET_BUTTON_WIDTH + 6 +
                                           List::ARROW_W,
                                           LIST_HEIGHT),
                                      ROW_HEIGHT);
        _window->addChild(merchantList);
        y += LIST_HEIGHT;

        for (size_t i = 0; i != objectType()->merchantSlots(); ++i){
            _merchantSlotElements[i] = new Element();
            merchantList->addChild(_merchantSlotElements[i]);
        }

    _window->resize(newWidth, y);
}

void ClientObject::addInventoryToWindow(){
    px_t
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();

    const size_t slots = objectType()->containerSlots();
    static const size_t COLS = 8;
    size_t rows = (slots - 1) / COLS + 1;
    Container *container = new Container(rows, COLS, _container, _serial, 0, y);
    _window->addChild(container);
    y += container->height();
    if (newWidth < container->width())
        newWidth = container->width();

    _window->resize(newWidth, y);
}

void ClientObject::addDeconstructionToWindow(){
    px_t
        x = BUTTON_GAP,
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();
    y += BUTTON_GAP;
    Button *deconstructButton = new Button(Rect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT),
                                            "Dismantle", startDeconstructing, this);
    _window->addChild(deconstructButton);
    y += BUTTON_GAP + BUTTON_HEIGHT;
    x += BUTTON_GAP + BUTTON_WIDTH;
    if (newWidth < x)
        newWidth = x;

    _window->resize(newWidth, y);
}

void ClientObject::addVehicleToWindow(){
    px_t
        x = BUTTON_GAP,
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();
    y += BUTTON_GAP;
    Button *mountButton = new Button(Rect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT), "Enter/exit",
                                            ClientVehicle::mountOrDismount, this);
    _window->addChild(mountButton);
    y += BUTTON_GAP + BUTTON_HEIGHT;
    x += BUTTON_GAP + BUTTON_WIDTH;
    if (newWidth < x)
        newWidth = x;

    _window->resize(newWidth, y);
}

void ClientObject::addMerchantTradeToWindow(){
    px_t
        y = _window->contentHeight(),
        newWidth = _window->contentWidth();

    static const px_t
        NAME_WIDTH = 100,
        QUANTITY_WIDTH = 20,
        BUTTON_PADDING = 1,
        BUTTON_LABEL_WIDTH = 45,
        BUTTON_HEIGHT = Element::ITEM_HEIGHT + 2 * BUTTON_PADDING,
        ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
        WIDTH = 2 * Element::ITEM_HEIGHT +
                2 * NAME_WIDTH +
                QUANTITY_WIDTH + 
                BUTTON_LABEL_WIDTH +
                2 * BUTTON_PADDING +
                3 * GAP +
                List::ARROW_W;
    const double
        MAX_ROWS = 7.5,
        NUM_ROWS = objectType()->merchantSlots() < MAX_ROWS ? objectType()->merchantSlots()
                                                            : MAX_ROWS;
    static const px_t
        HEIGHT = toInt(ROW_HEIGHT * NUM_ROWS);
    List *list = new List(Rect(0, 0, WIDTH, HEIGHT), ROW_HEIGHT);
    y += list->height();
    _window->addChild(list);
    for (size_t i = 0; i != objectType()->merchantSlots(); ++i){
        _merchantSlotElements[i] = new Element();
        list->addChild(_merchantSlotElements[i]);
    }
    if (newWidth < WIDTH)
        newWidth = WIDTH;

    _window->resize(newWidth, y);
}

void ClientObject::assembleWindow(Client &client){
    const ClientObjectType &objType = *objectType();

    static const px_t WINDOW_WIDTH = Container(1, 8, _container).width();

    if (_window != nullptr){
        _window->clearChildren();
        _window->resize(0, 0);
    }

    bool
        hasContainer = objType.containerSlots() > 0,
        isMerchant = objType.merchantSlots() > 0,
        isVehicle = classTag() == 'v';
    if (isMerchant ||
        userHasAccess() && (hasContainer ||
                            isVehicle ||
                            objType.canDeconstruct() ||
                            isBeingConstructed() )){

        if (_window == nullptr)
            _window = new Window(Rect(), objType.name());

        if (isBeingConstructed()){
            client.watchObject(*this);
            addConstructionToWindow();

        } else if (!userHasAccess() && isMerchant) {
            addMerchantTradeToWindow();

        } else {
            if (isMerchant)
                addMerchantSetupToWindow();
            if (hasContainer){
                client.watchObject(*this);
                addInventoryToWindow();
            }
            if (isVehicle)
                addVehicleToWindow();
            if (objType.canDeconstruct())
                addDeconstructionToWindow();
        }

    } else {
        if (_window != nullptr){
            _window->hide();
            client.removeWindow(_window);
            delete _window;
            _window = nullptr;
        }
    }
}

void ClientObject::playGatherSound() const {
    Mix_Chunk *sound = objectType()->gatherSound();
    if (sound != nullptr) {
        Mix_PlayChannel(Client::PLAYER_ACTION_CHANNEL, sound, -1);
    }
}

void ClientObject::onInventoryUpdate() {
    if (_window != nullptr)
        _window->forceRefresh();
}

void ClientObject::hideWindow() {
    if (_window != nullptr)
        _window->hide();
}

void ClientObject::startDeconstructing(void *object){
    const ClientObject &obj = *static_cast<const ClientObject *>(object);
    Client &client = *Client::_instance;
    client.sendMessage(CL_DECONSTRUCT, makeArgs(obj.serial()));
    client.prepareAction(std::string("Dismantling ") + obj.objectType()->name());
}

void ClientObject::trade(void *serialAndSlot){
    const std::pair<size_t, size_t> pair = *static_cast<std::pair<size_t, size_t>*>(serialAndSlot);
    Client::_instance->sendMessage(CL_TRADE, makeArgs(pair.first, pair.second));
}

void ClientObject::sendMerchantSlot(void *serialAndSlot){
    const std::pair<size_t, size_t> pair = *static_cast<std::pair<size_t, size_t>*>(serialAndSlot);
    size_t
        serial = pair.first,
        slot = pair.second;
    const auto &objects = Client::_instance->_objects;
    auto it = objects.find(serial);
    if (it == objects.end()){
        Client::debug()("Attempting to configure nonexistent object", Color::FAILURE);
        return;
    }
    ClientObject &obj = *it->second;
    ClientMerchantSlot &mSlot = obj._merchantSlots[slot];

    // Set quantities
    mSlot.wareQty = obj._wareQtyBoxes[slot]->textAsNum();
    mSlot.priceQty = obj._priceQtyBoxes[slot]->textAsNum();

    if (mSlot.wareItem == nullptr || mSlot.priceItem == nullptr){
        Client::debug()("You must select an item; clearing slot.", Color::WARNING);
        Client::_instance->sendMessage(CL_CLEAR_MERCHANT_SLOT, makeArgs(serial, slot));
        return;
    }

    Client::_instance->sendMessage(CL_SET_MERCHANT_SLOT,
                                   makeArgs(serial, slot,
                                            mSlot.wareItem->id(), mSlot.wareQty,
                                            mSlot.priceItem->id(), mSlot.priceQty));
}

bool ClientObject::userHasAccess() const{
    return
        _owner.empty() ||
        _owner == Client::_instance->username();
}

void ClientObject::update(double delta) {
    Client &client = *Client::_instance;

    // If being gathered, add particles.
    if (beingGathered())
        client.addParticles(objectType()->gatherParticles(), location(), delta);

    // If transforming, reduce timer.
    if (_transformTimer > 0){
        ms_t timeElapsed = toInt(1000 * delta);
        if (timeElapsed > _transformTimer)
            _transformTimer = 0;
        else
            _transformTimer -= timeElapsed;
    }
}

const Texture &ClientObject::cursor(const Client &client) const {
    const ClientObjectType &ot = *objectType();
    if (ot.canGather())
        return client.cursorGather();
    if (ot.containerSlots() > 0 || ot.merchantSlots() > 0)
        return client.cursorContainer();
    return client.cursorNormal();
}

const Texture &ClientObject::tooltip() const{
    if (_tooltip)
        return _tooltip;

    const ClientObjectType &ot = *objectType();

    bool isContainer = ot.containerSlots() > 0 && classTag() != 'n';

    // Name
    TooltipBuilder tb;
    tb.setColor(Color::ITEM_NAME);
    std::string title = ot.name();
    if (isBeingConstructed())
        title += " (under construction)";
    tb.addLine(title);

    // Debug info
    if (isDebug()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        tb.addLine("Serial: " + toString(_serial));
        tb.addLine("Class tag: " + toString(classTag()));
    }

    // Owner
    if (!owner().empty()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        tb.addLine("Owned by " + (owner() == Client::_instance->username() ? "you" : owner()));
    }

    // Stats
    bool stats = false;
    tb.setColor(Color::ITEM_STATS);

    if (classTag() == 'v'){
        if (!stats) {stats = true; tb.addGap(); }
        tb.addLine("Vehicle");
    }

    if (ot.canGather()){
        if (!stats) {stats = true; tb.addGap(); }
        std::string text = "Gatherable";
        if (!ot.gatherReq().empty())
            text += " (requires " + ot.gatherReq() + ")";
        tb.addLine(text);
    }

    if (ot.canDeconstruct()){
        if (!stats) {stats = true; tb.addGap(); }
        tb.addLine("Can dismantle");
    }

    if (isContainer){
        if (!stats) {stats = true; tb.addGap(); }
        tb.addLine("Container: " + toString(ot.containerSlots()) + " slots");
    }

    if (ot.merchantSlots() > 0){
        if (!stats) {stats = true; tb.addGap(); }
        tb.addLine("Merchant: " + toString(ot.merchantSlots()) + " slots");
    }

    // Tags
    if (ot.hasTags()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        for (const std::string &tag : ot.tags())
            tb.addLine(tag);
    }

    // Any actions available?
    if (ot.merchantSlots() > 0 || userHasAccess() && (classTag() == 'v' ||
                                                      isContainer ||
                                                      ot.canDeconstruct())){
        tb.addGap();
        tb.setColor(Color::ITEM_INSTRUCTIONS);
        tb.addLine(std::string("Right-click for controls"));
    }

    else if (classTag() == 'n'){
        tb.addGap();
        tb.setColor(Color::ITEM_INSTRUCTIONS);
        const ClientNPC &npc = dynamic_cast<const ClientNPC &>(*this);
        bool alive = npc.health() > 0;
        if (alive) tb.addLine("Right-click to attack");
        else if (npc.lootable()) tb.addLine("Right-click to loot");
    }

    else if (ot.canGather()){
        tb.addGap();
        tb.setColor(Color::ITEM_INSTRUCTIONS);
        tb.addLine(std::string("Right-click to gather"));
    }


    _tooltip = tb.publish();
    return _tooltip;
}

const Texture &ClientObject::image() const{
    if (isBeingConstructed())
        return objectType()->constructionImage().normal;
    if (objectType()->transforms())
        return objectType()->getProgressImage(_transformTimer).normal;
    return Entity::image();
}

const Texture &ClientObject::highlightImage() const{
    if (isBeingConstructed())
        return objectType()->constructionImage().highlight;
    if (objectType()->transforms())
        return objectType()->getProgressImage(_transformTimer).highlight;
    return Entity::highlightImage();
}
