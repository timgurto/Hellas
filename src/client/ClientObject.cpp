// (C) 2015 Tim Gurto

#include <cassert>

#include "ClientObject.h"
#include "Client.h"
#include "Renderer.h"
#include "ui/Button.h"
#include "ui/Container.h"
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
        _container = Item::vect_t(objectType()->containerSlots());
    }
}

ClientObject::~ClientObject(){
    if (_window) {
        Client::_instance->removeWindow(_window);
        delete _window;
    }
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
        if (userHasAccess() && !_window && (hasContainer || objType.canDeconstruct())){
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
                client.sendMessage(CL_GET_INVENTORY, makeArgs(_serial)); // Request inventory
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

bool ClientObject::userHasAccess() const{
    return
        _owner.empty() ||
        _owner == Client::_instance->username();
}
