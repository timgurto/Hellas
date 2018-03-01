#include "Action.h"
#include "../Server.h"

Action::FunctionMap Action::functionMap = {
    { "createCity", Server::createCity },
    { "setRespawnPoint", Server::setRespawnPoint }
};

CallbackAction::FunctionMap CallbackAction::functionMap = {
    { "destroyCity", Server::destroyCity }
};

void Server::createCity(const Object & obj, User & performer,
        const std::string &textArg) {
    auto &server = Server::instance();

    if (textArg == "_")
        return;

    if (!server._cities.getPlayerCity(performer.name()).empty()) {
        server.sendMessage(performer.socket(), WARNING_YOU_ARE_ALREADY_IN_CITY);
        return;
    }

    server._cities.createCity(textArg);
    server._cities.addPlayerToCity(performer, textArg);

    server.makePlayerAKing(performer);

    server.broadcast(SV_CITY_FOUNDED, makeArgs(performer.name(), textArg));
}

void Server::setRespawnPoint(const Object & obj, User & performer,
    const std::string &textArg) {
    performer.respawnPoint(obj.location());

    instance().sendMessage(performer.socket(), SV_SET_SPAWN);
}

void Server::destroyCity(const Object & obj) {
    auto owner = obj.permissions().owner();
    const auto cityName = instance()._cities.getPlayerCity(owner.name);
    instance()._cities.destroyCity(cityName);

    instance().broadcast(SV_CITY_DESTROYED, cityName);
}
