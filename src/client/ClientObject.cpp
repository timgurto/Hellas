// (C) 2015 Tim Gurto

#include <cassert>

#include "ClientObject.h"
#include "Client.h"
#include "Renderer.h"
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

    if (objectType()->canGather()) {
        std::ostringstream oss;
        client.sendMessage(CL_GATHER, makeArgs(_serial));
        client.prepareAction(std::string("Gathering ") + objectType()->name());
        playGatherSound();
    } else if (objectType()->containerSlots() > 0) {
        // Display window
        if (!_window) {
            static const int
                COLS = 8,
                DEFAULT_X = 100,
                DEFAULT_Y = 100;
            const size_t slots = objectType()->containerSlots();
            size_t rows = (slots - 1) / COLS + 1;
            Container *invElem = new Container(rows, COLS, _container, _serial);
            _window = new Window(Rect(DEFAULT_X, DEFAULT_Y, invElem->width(), invElem->height()),
                                 objectType()->name());
            _window->addChild(invElem);
            client.addWindow(_window);
        }
        _window->show();

        // Request inventory
        client.sendMessage(CL_GET_INVENTORY, makeArgs(_serial));
    }
}

std::vector<std::string> ClientObject::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back(objectType()->name());
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
