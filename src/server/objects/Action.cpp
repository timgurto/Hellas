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

    if (!server._cities.getPlayerCity(performer.name()).empty())
        return;

    server._cities.createCity(textArg);
    server._cities.addPlayerToCity(performer, textArg);

    server.makePlayerAKing(performer);
}

void Server::setRespawnPoint(const Object & obj, User & performer,
    const std::string &textArg) {
    performer.respawnPoint(obj.location());
}

void Server::destroyCity(const Object & obj) {
    auto owner = obj.permissions().owner();
    const auto &cityName = instance()._cities.getPlayerCity(owner.name);
    instance()._cities.destroyCity(cityName);
}
