// (C) 2015-2016 Tim Gurto

#include <SDL_mixer.h>
#include <cassert>

#include "ClientObject.h"
#include "Client.h"
#include "Renderer.h"
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

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial),
_container(rhs._container),
_window(nullptr){}

ClientObject::ClientObject(size_t serialArg, const EntityType *type, const Point &loc):
Entity(type, loc),
_serial(serialArg),
_window(nullptr){
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

    if (_window == nullptr)
        return;
    assert(_merchantSlotElements[i]);

    // Update slot element
    Element &e = *_merchantSlotElements[i];
    const ClientItem
        *wareItem = toClientItem(mSlot.wareItem),
        *priceItem = toClientItem(mSlot.priceItem);
    e.clearChildren();

    static const px_t // TODO: remove duplicate consts
        GAP = 2,
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

    // Make sure object is in range
    if (distance(client.playerCollisionRect(), collisionRect()) > Client::ACTION_DISTANCE) {
        client._debug("That object is too far away.", Color::MMO_HIGHLIGHT);
        return;
    }

    const ClientObjectType &objType = *objectType();
    if (objType.canGather() && userHasAccess()) {
        client.sendMessage(CL_GATHER, makeArgs(_serial));
        client.prepareAction(std::string("Gathering ") + objType.name());
        playGatherSound();
    } else {

        if (_window != nullptr){
            client.removeWindow(_window);
            client.addWindow(_window);
        }

        // Create window, if necessary
        bool
            hasContainer = objType.containerSlots() > 0,
            isMerchant = objType.merchantSlots() > 0;
        if (userHasAccess() && _window == nullptr &&
            (hasContainer || isMerchant || objType.canDeconstruct())){
            static const size_t COLS = 8;
            static const px_t
                WINDOW_WIDTH = Container(1, 8, _container).width(),
                BUTTON_HEIGHT = 15,
                BUTTON_WIDTH = 60,
                BUTTON_GAP = 1;
            px_t x = BUTTON_GAP, y = 0;
            px_t winWidth = 0;
            _window = new Window(Rect(0, 0, 0, 0), objType.name());
            client.addWindow(_window);

            // Merchant setup
            if (isMerchant){
                client.watchObject(*this);
                static const px_t
                    QUANTITY_WIDTH = 20,
                    NAME_WIDTH = 100,
                    GAP = 2,
                    PANE_WIDTH = Element::ITEM_HEIGHT + QUANTITY_WIDTH + NAME_WIDTH + 2 * GAP,
                    TITLE_HEIGHT = 14,
                    SET_BUTTON_WIDTH = 60,
                    ROW_HEIGHT = Element::ITEM_HEIGHT + 4 * GAP;
                static const double
                    MAX_ROWS = 5.5;
                const px_t
                    LIST_HEIGHT = toInt(ROW_HEIGHT * min(MAX_ROWS, objType.merchantSlots()));
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
                winWidth = max(winWidth, x);
                y += Element::TEXT_HEIGHT;
                List *merchantList = new List(Rect(0, y,
                                                   PANE_WIDTH * 2 + GAP * 5 + SET_BUTTON_WIDTH + 6 +
                                                   List::ARROW_W,
                                                   LIST_HEIGHT),
                                              ROW_HEIGHT);
                _window->addChild(merchantList);
                y += LIST_HEIGHT;

                for (size_t i = 0; i != objType.merchantSlots(); ++i){
                    _merchantSlotElements[i] = new Element();
                    merchantList->addChild(_merchantSlotElements[i]);
                }
            }

            // Inventory container
            if (hasContainer){
                client.watchObject(*this);
                const size_t slots = objType.containerSlots();
                size_t rows = (slots - 1) / COLS + 1;
                Container *container = new Container(rows, COLS, _container, _serial, 0, y);
                _window->addChild(container);
                y += container->height();
                winWidth = max(winWidth, container->width());
            }

            // Deconstruct button
            if (objType.canDeconstruct()){
                y += BUTTON_GAP;
                Button *deconstructButton = new Button(Rect(0, y, BUTTON_WIDTH, BUTTON_HEIGHT),
                                                       "Dismantle", startDeconstructing, this);
                _window->addChild(deconstructButton);
                // x += BUTTON_GAP + BUTTON_WIDTH;
                y += BUTTON_GAP + BUTTON_HEIGHT;
                winWidth = max(winWidth, x);
            }

            _window->resize(winWidth, y);

        } else if (!userHasAccess() && !_window && isMerchant) {
            client.watchObject(*this);
            // Draw trade window
            static const px_t
                GAP = 2,
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
                NUM_ROWS = objType.merchantSlots() < MAX_ROWS ? objType.merchantSlots() : MAX_ROWS;
            static const px_t
                HEIGHT = toInt(ROW_HEIGHT * NUM_ROWS);
            _window = new Window(Rect(0, 0, WIDTH, HEIGHT), objType.name());
            client.addWindow(_window);
            List *list = new List(Rect(0, 0, WIDTH, HEIGHT), ROW_HEIGHT);
            _window->addChild(list);
            for (size_t i = 0; i != objType.merchantSlots(); ++i){
                _merchantSlotElements[i] = new Element();
                list->addChild(_merchantSlotElements[i]);
            }
        }

        if (_window != nullptr) {
            // Determine placement: center around object, but keep entirely on screen.
            px_t x = toInt(location().x - _window->width() / 2 + client.offset().x);
            x = max(0, min(x, Client::SCREEN_X - _window->width()));
            px_t y = toInt(location().y - _window->height() / 2 + client.offset().y);
            y = max(0, min(y, Client::SCREEN_Y - _window->height()));
            _window->rect(x, y);

            _window->show();
        }
    }
}

std::vector<std::string> ClientObject::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(objectType()->name());
    if (isDebug())
        text.push_back("Serial: " + toString(_serial));
    if (!_owner.empty())
        text.push_back(std::string("Owned by ") + _owner);
    return text;
}

void ClientObject::playGatherSound() const {
    Mix_Chunk *sound = objectType()->gatherSound();
    if (sound != nullptr) {
        Mix_PlayChannel(Client::PLAYER_ACTION_CHANNEL, sound, -1);
    }
}

void ClientObject::draw(const Client &client) const{
    assert(type());
    // Highilght moused-over entity
    if (this == client.currentMouseOverEntity()) {
        if (distance(collisionRect(), client.playerCollisionRect()) <= Client::ACTION_DISTANCE)
            renderer.setDrawColor(Color::MMO_L_GREEN);
        else
            renderer.setDrawColor(Color::MMO_SKIN);
        renderer.drawRect(collisionRect() + Rect(-1, -1, 2, 2) + client.offset());
    }

    type()->drawAt(location() + client.offset());

    if (isDebug()) {
        renderer.setDrawColor(Color::GREY_2);
        renderer.drawRect(collisionRect() + client.offset());
        renderer.setDrawColor(Color::YELLOW);
        renderer.fillRect(Rect(location().x + client.offset().x,
                                location().y + client.offset().y - 1,
                                1, 3));
        renderer.fillRect(Rect(location().x + client.offset().x - 1,
                               location().y + client.offset().y,
                               3, 1));
    }
}

void ClientObject::refreshWindow() {
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
        Client::debug()("Attempting to configure nonexistent object", Color::MMO_RED);
        return;
    }
    ClientObject &obj = *it->second;
    ClientMerchantSlot &mSlot = obj._merchantSlots[slot];

    // Set quantities
    mSlot.wareQty = obj._wareQtyBoxes[slot]->textAsNum();
    mSlot.priceQty = obj._priceQtyBoxes[slot]->textAsNum();

    if (mSlot.wareItem == nullptr || mSlot.priceItem == nullptr){
        Client::debug()("You must select an item; clearing slot.", Color::MMO_HIGHLIGHT);
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
