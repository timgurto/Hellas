#include "Action.h"

#include "../Server.h"

Action::FunctionMap Action::functionMap = {
    {"endTutorial", Server::endTutorial},
    {"createCity", Server::createCity},
    {"setRespawnPoint", Server::setRespawnPoint}};

CallbackAction::FunctionMap CallbackAction::functionMap = {
    {"destroyCity", Server::destroyCity}};

bool Server::endTutorial(const Object &obj, User &performer,
                         const std::string &textArg) {
  auto &server = Server::instance();

  performer.exploration().unexploreAll(performer.socket());

  performer.setSpawnPointToPostTutorial();
  performer.moveToSpawnPoint();

  performer.getClass().unlearnAll();

  performer.clearInventory();
  performer.clearGear();

  // Replenish the usage cost, after clearing the inventory.  Do it directly,
  // rather than via User::giveItem(), to avoid alerting the client
  // unnecessarily.
  const auto costItem = obj.objType().action().cost;
  if (costItem) {
    auto &inventorySlot = performer.inventory(0);
    inventorySlot.first = {costItem};
    inventorySlot.second = 1;
  }

  performer.removeConstruction("tutFire");
  performer.addConstruction("fire");
  server.sendMessage(performer.socket(), SV_CONSTRUCTIONS, makeArgs(1, "fire"));

  server.removeAllObjectsOwnedBy(
      {Permissions::Owner::PLAYER, performer.name()});

  return true;
}

bool Server::createCity(const Object &obj, User &performer,
                        const std::string &textArg) {
  auto &server = Server::instance();

  if (textArg == "_") return false;

  if (!server._cities.getPlayerCity(performer.name()).empty()) {
    performer.sendMessage(WARNING_YOU_ARE_ALREADY_IN_CITY);
    return false;
  }

  server._cities.createCity(textArg);
  server._cities.addPlayerToCity(performer, textArg);

  server.makePlayerAKing(performer);

  server.broadcast(SV_CITY_FOUNDED, makeArgs(performer.name(), textArg));
  return true;
}

bool Server::setRespawnPoint(const Object &obj, User &performer,
                             const std::string &textArg) {
  performer.respawnPoint(obj.location());

  performer.sendMessage(SV_SET_SPAWN);
  return true;
}

void Server::destroyCity(const Object &obj) {
  auto owner = obj.permissions().owner();
  const auto cityName = instance()._cities.getPlayerCity(owner.name);
  instance()._cities.destroyCity(cityName);

  instance().broadcast(SV_CITY_DESTROYED, cityName);
}
