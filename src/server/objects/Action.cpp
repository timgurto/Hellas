#include "Action.h"
#include "../Server.h"

Action::FunctionMap Action::functionMap = {
    { "createCity", Server::createCity },
    { "setRespawnPoint", Server::setRespawnPoint }
};

void Server::createCity(const Object & obj, User & performer,
        const std::string &textArg) {
    auto &server = Server::instance();

    if (textArg == "_")
        return;

    server._cities.createCity(textArg);
    server._cities.addPlayerToCity(performer, textArg);

    server.makePlayerAKing(performer);
}

void Server::setRespawnPoint(const Object & obj, User & performer,
        const std::string &textArg) {
    performer.respawnPoint(obj.location());
}
