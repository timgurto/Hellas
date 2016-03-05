// (C) 2015 Tim Gurto

#include <cassert>

#include "ClientObject.h"
#include "Client.h"
#include "Renderer.h"
#include "ui/Button.h"
#include "ui/Container.h"
#include "ui/Label.h"
#include "ui/Window.h"
#include "../Color.h"
#include "../Log.h"
#include "../util.h"

extern Renderer renderer;

ClientObject::ClientObject(const ClientObject &rhs):
Entity(rhs),
_serial(rhs._serial),
_container(rhs._container),
_window(0){}

ClientObject::ClientObject(size_t serialArg, const EntityType *type, const Point &loc):
Entity(type, loc),
_serial(serialArg),
_window(0){
    if (type) { // i.e., not a serial-only search dummy
        const size_t
            containerSlots = objectType()->containerSlots(),
            merchantSlots = objectType()->merchantSlots();
        _container = Item::vect_t(containerSlots);
        _merchantSlots = std::vector<MerchantSlot>(merchantSlots);
        _merchantSlotElements = std::vector<Element *>(merchantSlots, 0);
        _serialSlotPairs = std::vector<serialSlotPair_t *>(merchantSlots, 0);
        for (size_t i = 0; i != merchantSlots; ++i){
            serialSlotPair_t *pair = new serialSlotPair_t();
            pair->first = _serial;
            pair->second = i;
            _serialSlotPairs[i] = pair;
        }
    }
}

ClientObject::~ClientObject(){
    for (auto p : _serialSlotPairs)
        delete p;
    if (_window) {
        Client::_instance->removeWindow(_window);
        delete _window;
    }
}

void ClientObject::setMerchantSlot(size_t i, const MerchantSlot &mSlot){
    _merchantSlots[i] = mSlot;

    if (!_window)
        return;
    assert(_merchantSlotElements[i]);

    // Update slot element
    Element &e = *_merchantSlotElements[i];
    if (!mSlot) {
        e = Element();
        return;
    }

    static const int // TODO: remove duplicate consts
        GAP = 2,
        NAME_WIDTH = 100,
        QUANTITY_WIDTH = 20,
        BUTTON_PADDING = 1,
        TEXT_HEIGHT = Element::TEXT_HEIGHT,
        ICON_SIZE = Element::ITEM_HEIGHT,
        BUTTON_LABEL_WIDTH = 45,
        BUTTON_HEIGHT = ICON_SIZE + 2 * BUTTON_PADDING,
        BUTTON_WIDTH = BUTTON_PADDING * 2 + BUTTON_LABEL_WIDTH + ICON_SIZE + NAME_WIDTH,
        ROW_HEIGHT = BUTTON_HEIGHT + 2 * GAP,
        TEXT_TOP = (ROW_HEIGHT - TEXT_HEIGHT) / 2,
        BUTTON_TEXT_TOP = (BUTTON_HEIGHT - TEXT_HEIGHT) / 2;

    // Ware
    int x = GAP;
    e.addChild(new Label(Rect(x, TEXT_TOP, QUANTITY_WIDTH, TEXT_HEIGHT),
                         makeArgs(mSlot.wareQty()), Element::RIGHT_JUSTIFIED));
    x += QUANTITY_WIDTH;
    e.addChild(new Picture(Rect(x, (ROW_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE),
                           mSlot.wareItem()->icon()));
    x += ICON_SIZE;
    e.addChild(new Label(Rect(x, TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT), mSlot.wareItem()->name()));
    x += NAME_WIDTH + GAP;
    
    // Buy button
    Button *button = new Button(Rect(x, GAP, BUTTON_WIDTH, BUTTON_HEIGHT), "", trade,
                                _serialSlotPairs[i]);
    e.addChild(button);
    x = BUTTON_PADDING;
    button->addChild(new Label(Rect(x, BUTTON_TEXT_TOP, BUTTON_LABEL_WIDTH, TEXT_HEIGHT),
                               std::string("Buy for ") + makeArgs(mSlot.priceQty()),
                               Element::RIGHT_JUSTIFIED));
    x += BUTTON_LABEL_WIDTH;
    button->addChild(new Picture(Rect(x, BUTTON_PADDING, ICON_SIZE, ICON_SIZE),
                                 mSlot.priceItem()->icon()));
    x += ICON_SIZE;
    button->addChild(new Label(Rect(x, BUTTON_TEXT_TOP, NAME_WIDTH, TEXT_HEIGHT),
                               mSlot.priceItem()->name()));
}

void ClientObject::onRightClick(Client &client){
    //Client::debug() << "Right-clicked on object #" << _serial << Log::endl;

    // Make sure object is in range
    if (distance(client.playerCollisionRect(), collisionRect()) > Client::ACTION_DISTANCE) {
        client._debug("That object is too far away.", Color::YELLOW);
        return;
    }

    const ClientObjectType &objType = *objectType();
    if (objType.canGather() && userHasAccess()) {
        client.sendMessage(CL_GATHER, makeArgs(_serial));
        client.prepareAction(std::string("Gathering ") + objType.name());
        playGatherSound();
    } else {

        if (_window){
            client.removeWindow(_window);
            client.addWindow(_window);
        }

        // Create window, if necessary
        bool
            hasContainer = objType.containerSlots() > 0,
            isMerchant = objType.merchantSlots() > 0;
        if (userHasAccess() && !_window && (hasContainer || isMerchant || objType.canDeconstruct())){
            static const size_t COLS = 8;
            static const int
                WINDOW_WIDTH = Container(1, 8, _container).width(),
                BUTTON_HEIGHT = 15,
                BUTTON_WIDTH = 60,
                BUTTON_GAP = 1;
            int x = BUTTON_GAP, y = 0;
            int winWidth = 0;
            _window = new Window(Rect(0, 0, 0, 0), objType.name());
            client.addWindow(_window);

            // Merchant setup
            if (isMerchant){
                
            }

            // Inventory container
            if (hasContainer){
                const size_t slots = objType.containerSlots();
                size_t rows = (slots - 1) / COLS + 1;
                Container *container = new Container(rows, COLS, _container, _serial);
                client.watchObject(*this);
                _window->addChild(container);
                y += container->height();
                winWidth = max(winWidth, container->width());
            }

            // Deconstruct button
            if (objType.canDeconstruct()){
                y += BUTTON_GAP;
                Button *deconstructButton = new Button(Rect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT),
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
            static const int
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
            static const int
                HEIGHT = toInt(ROW_HEIGHT * NUM_ROWS);
            _window = new Window(Rect(0, 0, WIDTH, HEIGHT), objType.name());
            client.addWindow(_window);
            List *list = new List(Rect(0, 0, WIDTH, HEIGHT), ROW_HEIGHT);
            _window->addChild(list);
            for (size_t i = 0; i != objType.merchantSlots(); ++i){
                const MerchantSlot &mSlot = _merchantSlots[i];
                _merchantSlotElements[i] = new Element();
                list->addChild(_merchantSlotElements[i]);
            }
        }

        if (_window) {
            // Determine placement: center around object, but keep entirely on screen.
            int x = toInt(location().x - _window->width() / 2 + client.offset().x);
            x = max(0, min(x, Client::SCREEN_X - _window->width()));
            int y = toInt(location().y - _window->height() / 2 + client.offset().y);
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
        text.push_back("Serial: " + makeArgs(_serial));
    if (!_owner.empty())
        text.push_back(std::string("Owned by ") + _owner);
    return text;
}

void ClientObject::playGatherSound() const {
    Mix_Chunk *sound = objectType()->gatherSound();
    if (sound) {
        Mix_PlayChannel(Client::PLAYER_ACTION_CHANNEL, sound, -1);
    }
}

void ClientObject::draw(const Client &client) const{
    assert(type());
    // Highilght moused-over entity
    if (this == client.currentMouseOverEntity()) {
        if (distance(collisionRect(), client.playerCollisionRect()) <= Client::ACTION_DISTANCE)
            renderer.setDrawColor(Color::BLUE/2 + Color::WHITE/2);
        else
            renderer.setDrawColor(Color::GREY_2);
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
    if (_window)
        _window->forceRefresh();
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

bool ClientObject::userHasAccess() const{
    return
        _owner.empty() ||
        _owner == Client::_instance->username();
}
