#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#ifndef SINGLE_THREAD
#include <mutex>
#include <thread>
#endif

#include "../Socket.h"
#include "../messageCodes.h"
#include "../util.h"
#include "../versionUtil.h"
#include "ItemSet.h"
#include "NPC.h"
#include "NPCType.h"
#include "ProgressLock.h"
#include "Recipe.h"
#include "Server.h"
#include "User.h"
#include "Vehicle.h"
#include "VehicleType.h"
#include "objects/Object.h"
#include "objects/ObjectType.h"

using namespace std::string_literals;

extern Args cmdLineArgs;

Server *Server::_instance = nullptr;
LogConsole *Server::_debugInstance = nullptr;

const int Server::MAX_CLIENTS = 20;

const ms_t Server::CLIENT_TIMEOUT = 10000;
const ms_t Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 300;

const px_t Server::ACTION_DISTANCE = Podes{4}.toPixels();
const px_t Server::CULL_DISTANCE = 450;
const px_t Server::TILE_W = 32;
const px_t Server::TILE_H = 32;

Server::Server()
    : _time(SDL_GetTicks()),
      _lastTime(_time),
      _socket(),
      _loop(false),
      _running(false),
      _mapX(0),
      _mapY(0),
      _debug("server.log"),
      _userFilesPath("Users/"),
      _lastSave(_time),
      _timeStatsLastPublished(_time),
      _dataLoaded(false) {
  _instance = this;
  _debugInstance = &_debug;
  if (cmdLineArgs.contains("quiet")) _debug.quiet();

  _debug("Server v" + version());

  _debug << cmdLineArgs << Log::endl;
  Socket::debug = &_debug;

  User::init();
  NPCType::init();

  if (cmdLineArgs.contains("user-files-path"))
    _userFilesPath = cmdLineArgs.getString("user-files-path") + "/";
  if (cmdLineArgs.contains("new")) deleteUserFiles();

  _debug("Server initialized");

  // Socket details
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  // Select port
#ifdef _DEBUG
  auto port = DEBUG_PORT;
#else
  auto port = PRODUCTION_PORT;
#endif
  if (cmdLineArgs.contains("port")) port = cmdLineArgs.getInt("port");
  serverAddr.sin_port = htons(port);

  _socket.bind(serverAddr);
  _debug << "Server address: " << inet_ntoa(serverAddr.sin_addr) << ":"
         << ntohs(serverAddr.sin_port) << Log::endl;
  _socket.listen();

  _debug("Ready for connections");
}

Server::~Server() {
  saveData(_entities, _wars, _cities);
  for (auto pair : _terrainTypes) delete pair.second;
  for (const auto &spellPair : _spells) delete spellPair.second;

  _instance = nullptr;

  Socket::debug = nullptr;
}

void Server::checkSockets() {
  // Populate socket list with active sockets
  static fd_set readFDs;
  FD_ZERO(&readFDs);
  FD_SET(_socket.getRaw(), &readFDs);
  for (const Socket &socket : _clientSockets) {
    FD_SET(socket.getRaw(), &readFDs);
  }

  // Poll for activity
  static const timeval selectTimeout = {0, 10000};
  int activity = select(0, &readFDs, nullptr, nullptr, &selectTimeout);
  if (activity == SOCKET_ERROR) {
    _debug << Color::TODO << "Error polling sockets: " << WSAGetLastError()
           << Log::endl;
    return;
  }
  _time = SDL_GetTicks();

  // Activity on server socket: new connection
  if (FD_ISSET(_socket.getRaw(), &readFDs)) {
    if (_clientSockets.size() == MAX_CLIENTS) {
      _debug("No room for additional clients; all slots full");
      sockaddr_in clientAddr;
      SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr *)&clientAddr,
                                 (int *)&Socket::sockAddrSize);
      Socket s(tempSocket);
      // Allow time for rejection message to be sent before closing socket
      s.delayClosing(5000);
      sendMessage(s, WARNING_SERVER_FULL);
    } else {
      sockaddr_in clientAddr;
      SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr *)&clientAddr,
                                 (int *)&Socket::sockAddrSize);
      if (tempSocket == SOCKET_ERROR) {
        _debug << Color::TODO
               << "Error accepting connection: " << WSAGetLastError()
               << Log::endl;
      } else {
        _debug << Color::TODO
               << "Connection accepted: " << inet_ntoa(clientAddr.sin_addr)
               << ":" << ntohs(clientAddr.sin_port)
               << ", socket number = " << tempSocket << Log::endl;
        _clientSockets.insert(tempSocket);
      }
    }
  }

  // Activity on client socket: message received or client disconnected
  for (std::set<Socket>::iterator it = _clientSockets.begin();
       it != _clientSockets.end();) {
    SOCKET raw = it->getRaw();
    if (FD_ISSET(raw, &readFDs)) {
      sockaddr_in clientAddr;
      getpeername(raw, (sockaddr *)&clientAddr, (int *)&Socket::sockAddrSize);
      static char buffer[BUFFER_SIZE + 1];
      const int charsRead = recv(raw, buffer, BUFFER_SIZE, 0);
      if (charsRead == SOCKET_ERROR) {
        int err = WSAGetLastError();
        _debug << "Client " << raw << " disconnected; error code: " << err
               << Log::endl;
        removeUser(raw);
        closesocket(raw);
        _clientSockets.erase(it++);
        continue;
      } else if (charsRead == 0) {
        // Client disconnected
        _debug << "Client " << raw << " disconnected" << Log::endl;
        removeUser(*it);
        closesocket(raw);
        _clientSockets.erase(it++);
        continue;
      } else {
        // Message received
        buffer[charsRead] = '\0';
        _messages.push(std::make_pair(*it, std::string(buffer)));
      }
    }
    ++it;
  }
}

void Server::run() {
  if (!_dataLoaded) DataLoader::FromPath(*this).load();
  initialiseData();
  loadWorldState();
  spawnInitialObjects();

  auto threadsOpen = 0;

  _loop = true;
  _running = true;
  _debug("Server is running", Color::TODO);
  while (_loop) {
    _time = SDL_GetTicks();
    const ms_t timeElapsed = _time - _lastTime;
    _lastTime = _time;

    // Check that clients are alive
    for (std::set<User>::iterator it = _users.begin(); it != _users.end();) {
      if (!it->alive()) {
        _debug << Color::TODO << "User " << it->name() << " has timed out."
               << Log::endl;
        std::set<User>::iterator next = it;
        ++next;

        auto socketIt = _clientSockets.find(it->socket());
        assert(socketIt != _clientSockets.end());
        _clientSockets.erase(socketIt);

        removeUser(it);
        it = next;
      } else {
        ++it;
      }
    }

    // Save data
    if (_time - _lastSave >= SAVE_FREQUENCY) {
      for (const User &user : _users) {
        writeUserData(user);
      }

      std::thread(saveData, _entities, _wars, _cities).detach();

      _lastSave = _time;
    }

    // Publish stats
    if (_time - _timeStatsLastPublished >= PUBLISH_STATS_FREQUENCY) {
      std::thread(publishStats, this).detach();

      _timeStatsLastPublished = _time;
    }

    // Update users
    for (const User &user : _users)
      const_cast<User &>(user).update(timeElapsed);

    // Update non-user entities
    for (Entity *entP : _entities) entP->update(timeElapsed);

    // Clean up dead objects
    for (Entity *entP : _entitiesToRemove) {
      removeEntity(*entP);
    }
    _entitiesToRemove.clear();

    // Update spawners
    for (auto &spawner : _spawners) spawner.update(_time);

    // Deal with any messages from the server
    while (!_messages.empty()) {
      handleMessage(_messages.front().first, _messages.front().second);
      _messages.pop();
    }

    checkSockets();

    SDL_Delay(1);
  }

  // Save all user data
  for (const User &user : _users) {
    writeUserData(user);
  }
  _running = false;

  while (_threadsOpen > 0)
    ;
}

void Server::addUser(const Socket &socket, const std::string &name,
                     const std::string &classID) {
  auto newUserToInsert = User{name, {}, socket};

  // Add new user to list
  std::set<User>::const_iterator it = _users.insert(newUserToInsert).first;
  auto &newUser = const_cast<User &>(*it);
  _usersByName[name] = &*it;

  const bool userExisted = readUserData(newUser);
  if (!userExisted) {
    newUser.setClass(_classes[classID]);
    newUser.moveToSpawnPoint(true);
    _debug << "New";
  } else {
    _debug << "Existing";
    newUser.updateStats();
  }
  _debug << " user, " << name << " has logged in." << Log::endl;

  sendMessage(socket, SV_WELCOME);

  // Calculate and send him his stats
  newUser.updateStats();

  newUser.sendInfoToClient(newUser);
  _wars.sendWarsToUser(newUser, *this);

  for (const User *userP : findUsersInArea(newUser.location())) {
    if (userP == &newUser) continue;

    // Send him information about other nearby users
    userP->sendInfoToClient(newUser);
    // Send nearby others this user's information
    newUser.sendInfoToClient(*userP);
  }

  sendRelevantEntitiesToUser(newUser);

  // Send him his inventory
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    if (newUser.inventory(i).first != nullptr)
      sendInventoryMessage(newUser, i, INVENTORY);
  }

  // Send him his gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    if (newUser.gear(i).first != nullptr)
      sendInventoryMessage(newUser, i, GEAR);
  }

  // Send him the recipes he knows
  if (newUser.knownRecipes().size() > 0) {
    std::string args = makeArgs(newUser.knownRecipes().size());
    for (const std::string &id : newUser.knownRecipes()) {
      args = makeArgs(args, id);
    }
    newUser.sendMessage(SV_RECIPES, args);
  }

  // Send him the constructions he knows
  if (newUser.knownConstructions().size() > 0) {
    std::string args = makeArgs(newUser.knownConstructions().size());
    for (const std::string &id : newUser.knownConstructions()) {
      args = makeArgs(args, id);
    }
    newUser.sendMessage(SV_CONSTRUCTIONS, args);
  }

  // Send him his talents
  const auto &userClass = newUser.getClass();
  auto treesToSend = std::set<std::string>{};
  for (auto pair : userClass.talentRanks()) {
    const auto &talent = *pair.first;
    newUser.sendMessage(SV_TALENT, makeArgs(talent.name(), pair.second));
    treesToSend.insert(talent.tree());
  }
  for (const auto &tree : treesToSend) {
    auto pointsInTree = userClass.pointsInTree(tree);
    newUser.sendMessage(SV_POINTS_IN_TREE, makeArgs(tree, pointsInTree));
  }

  // Send him his known spells
  auto knownSpellsString = userClass.generateKnownSpellsString();
  newUser.sendMessage(SV_KNOWN_SPELLS, knownSpellsString);
  newUser.getClass().teachSpell("sprint");
  newUser.getClass().teachSpell("blink");
  newUser.getClass().teachSpell("waterWalking");

  // Add user to location-indexed trees
  getCollisionChunk(newUser.location()).addEntity(&newUser);
  _usersByX.insert(&newUser);
  _usersByY.insert(&newUser);
  _entitiesByX.insert(&newUser);
  _entitiesByY.insert(&newUser);

  newUser.markAsInitialised();
}

void Server::removeUser(const std::set<User>::iterator &it) {
  const auto &userToDelete = *it;
  // Alert nearby users
  for (const User *userP : findUsersInArea(userToDelete.location()))
    if (userP != &userToDelete)
      userP->sendMessage(SV_USER_DISCONNECTED, userToDelete.name());

  forceAllToUntarget(userToDelete);

  for (auto *e : _entities) e->removeWatcher(it->name());

  // Save user data
  writeUserData(userToDelete);

  getCollisionChunk(userToDelete.location())
      .removeEntity(userToDelete.serial());
  _usersByX.erase(&userToDelete);
  _usersByY.erase(&userToDelete);
  _entitiesByX.erase(&userToDelete);
  _entitiesByY.erase(&userToDelete);
  _usersByName.erase(it->name());

  _users.erase(it);
}

void Server::removeUser(const Socket &socket) {
  const std::set<User>::iterator it = _users.find(socket);
  if (it != _users.end())
    removeUser(it);
  else
    _debug("User was already removed", Color::TODO);
}

std::set<User *> Server::findUsersInArea(MapPoint loc,
                                         double squareRadius) const {
  static User dummy{MapPoint{0}};
  std::set<User *> users;
  dummy.changeDummyLocation(loc.x - squareRadius);
  auto loX = _usersByX.lower_bound(&dummy);
  dummy.changeDummyLocation(loc.x + squareRadius);
  auto hiX = _usersByX.upper_bound(&dummy);
  for (auto it = loX; it != hiX; ++it)
    if (abs(loc.y - (*it)->location().y) <= squareRadius)
      users.insert(const_cast<User *>(*it));

  return users;
}

std::set<Entity *> Server::findEntitiesInArea(MapPoint loc,
                                              double squareRadius) const {
  std::set<Entity *> entities;
  auto loX =
      _entitiesByX.lower_bound(&Dummy::Location({loc.x - squareRadius, 0}));
  auto hiX =
      _entitiesByX.upper_bound(&Dummy::Location({loc.x + squareRadius, 0}));
  for (auto it = loX; it != hiX; ++it)
    if (abs(loc.y - (*it)->location().y) <= squareRadius)
      entities.insert(const_cast<Entity *>(*it));

  return entities;
}

bool Server::isEntityInRange(const Socket &client, const User &user,
                             const Entity *ent,
                             bool suppressErrorMessages) const {
  // Doesn't exist
  if (ent == nullptr) {
    if (!suppressErrorMessages) sendMessage(client, WARNING_DOESNT_EXIST);
    return false;
  }

  // Check distance from user
  if (distance(user.collisionRect(), ent->collisionRect()) > ACTION_DISTANCE) {
    if (!suppressErrorMessages) sendMessage(client, WARNING_TOO_FAR);
    return false;
  }

  return true;
}

void Server::forceAllToUntarget(const Entity &target,
                                const User *userToExclude) {
  // Fix users targeting the entity
  size_t serial = target.serial();
  for (const User &constUser : _users) {
    User &user = const_cast<User &>(constUser);
    if (&user == userToExclude) continue;
    if (user.target() == &target) {
      if (user.action() == User::ATTACK) user.finishAction();
      user.target(nullptr);
      continue;
    }
    if (user.action() == User::GATHER &&
        user.actionObject()->serial() == serial) {
      user.sendMessage(WARNING_DOESNT_EXIST);
      user.cancelAction();
      user.target(nullptr);
    }
  }

  for (const Entity *pEnt : _entities) {
    Entity &entity = *const_cast<Entity *>(pEnt);

    // Fix buffs cast by entity
    for (auto &buff : entity.buffs()) buff.clearCasterIfEqualTo(target);
    for (auto &debuff : entity.debuffs()) debuff.clearCasterIfEqualTo(target);

    // Fix NPCs targeting/aware of the entity
    if (entity.classTag() != 'n') continue;
    NPC &npc = dynamic_cast<NPC &>(entity);
    npc.forgetAbout(target);
    if (npc.target() && npc.target() == &target) npc.target(nullptr);
  }
}

void Server::removeEntity(Entity &ent, const User *userToExclude) {
  // Ensure no other users are targeting this object, as it will be removed.
  forceAllToUntarget(ent, userToExclude);

  // Alert nearby users of the removal
  size_t serial = ent.serial();
  for (const User *userP : findUsersInArea(ent.location()))
    userP->sendMessage(SV_REMOVE_OBJECT, makeArgs(serial));

  getCollisionChunk(ent.location()).removeEntity(serial);
  _entitiesByX.erase(&ent);
  _entitiesByY.erase(&ent);
  auto numRemoved = _entities.erase(&ent);
  delete &ent;
  assert(numRemoved == 1);
}

void Server::gatherObject(size_t serial, User &user) {
  // Give item to user
  Object *obj = _entities.find<Object>(serial);
  const ServerItem *const toGive = obj->chooseGatherItem();
  size_t qtyToRemove = obj->chooseGatherQuantity(toGive);
  size_t qtyToGive = qtyToRemove;
  if (user.shouldGatherDoubleThisTime()) qtyToGive *= 2;
  const size_t remaining = user.giveItem(toGive, qtyToGive);
  if (remaining > 0) {
    user.sendMessage(WARNING_INVENTORY_FULL);
    qtyToRemove -= remaining;
  }
  if (remaining <
      qtyToGive)  // User received something: trigger any new unlocks
    ProgressLock::triggerUnlocks(user, ProgressLock::GATHER, toGive);

  // Remove object if empty
  obj->removeItem(toGive, qtyToRemove);
  if (obj->contents().isEmpty()) {
    if (obj->objType().transformsOnEmpty()) {
      forceAllToUntarget(*obj);
      obj->removeAllGatheringUsers();
    } else
      removeEntity(*obj, &user);
  } else
    obj->decrementGatheringUsers();

#ifdef SINGLE_THREAD
  saveData(_objects, _wars, _cities);
#else
  std::thread(saveData, _entities, _wars, _cities).detach();
#endif
}

void Server::spawnInitialObjects() {
  // From spawners
  auto timeOfLastReport = SDL_GetTicks();

  auto numSpawners = _spawners.size();
  auto i = 0;
  for (auto &spawner : _spawners) {
    assert(spawner.type() != nullptr);
    for (size_t i = 0; i != spawner.quantity(); ++i) spawner.spawn();
    ++i;

    const auto REPORTING_TIME = 500;
    auto currentTime = SDL_GetTicks();
    if (currentTime - timeOfLastReport >= REPORTING_TIME) {
      timeOfLastReport = currentTime;
      _debug << "Loading spawners: " << i << "/" << numSpawners << " ("
             << 100 * i / numSpawners << "%)" << Log::endl;
    }
  }
}

MapPoint Server::mapRand() const {
  return {randDouble() * (_mapX - 0.5) * TILE_W, randDouble() * _mapY * TILE_H};
}

bool Server::itemIsTag(const ServerItem *item,
                       const std::string &tagName) const {
  assert(item);
  return item->isTag(tagName);
}

ObjectType *Server::findObjectTypeByName(const std::string &id) const {
  for (auto *type : _objectTypes)
    if (type->id() == id) return const_cast<ObjectType *>(type);
  return nullptr;
}

Object &Server::addObject(const ObjectType *type, const MapPoint &location,
                          const std::string &owner) {
  Object *newObj =
      type->classTag() == 'v'
          ? new Vehicle(dynamic_cast<const VehicleType *>(type), location)
          : new Object(type, location);
  if (!owner.empty()) {
    newObj->permissions().setPlayerOwner(owner);
    _objectsByOwner.add(Permissions::Owner(Permissions::Owner::PLAYER, owner),
                        newObj->serial());

    const auto *user = this->getUserByName(owner);
    if (user != nullptr) user->onNewOwnedObject(*type);
  }
  return dynamic_cast<Object &>(addEntity(newObj));
}

NPC &Server::addNPC(const NPCType *type, const MapPoint &location) {
  NPC *newNPC = new NPC(type, location);
  return dynamic_cast<NPC &>(addEntity(newNPC));
}

Entity &Server::addEntity(Entity *newEntity) {
  _entities.insert(newEntity);
  const MapPoint &loc = newEntity->location();

  // Alert nearby users
  for (const User *userP : findUsersInArea(loc))
    newEntity->sendInfoToClient(*userP);

  // Add object to relevant chunk
  if (newEntity->type()->collides())
    getCollisionChunk(loc).addEntity(newEntity);

  // Add object to x/y index sets
  _entitiesByX.insert(newEntity);
  _entitiesByY.insert(newEntity);

  return const_cast<Entity &>(*newEntity);
}

User *Server::getUserByName(const std::string &username) {
  auto it = _usersByName.find(username);
  if (it == _usersByName.end()) return nullptr;
  auto ptr = _usersByName.find(username)->second;
  return const_cast<User *>(ptr);
}

const BuffType *Server::getBuffByName(const Buff::ID &id) const {
  auto it = _buffTypes.find(id);
  if (it == _buffTypes.end()) return nullptr;
  return &it->second;
}

const Quest *Server::findQuest(const Quest::ID &id) const {
  auto it = _quests.find(id);
  if (it == _quests.end()) return nullptr;
  return &it->second;
}

const ServerItem *Server::findItem(const std::string &id) const {
  auto dummy = ServerItem{id};
  auto it = _items.find(dummy);
  if (it == _items.end()) return nullptr;
  return &*it;
}

const BuffType *Server::findBuff(const BuffType::ID &id) const {
  auto it = _buffTypes.find(id);
  if (it == _buffTypes.end()) return nullptr;
  return &it->second;
}

const Terrain *Server::terrainType(char index) const {
  auto &types = const_cast<std::map<char, Terrain *> &>(_terrainTypes);
  return types[index];
}

void Server::deleteUserFiles() {
  WIN32_FIND_DATAW fd;
  std::wstring path(_userFilesPath.begin(), _userFilesPath.end());
  std::replace(path.begin(), path.end(), '/', '\\');
  std::wstring filter = path + L"*.usr";
  path.c_str();
  HANDLE hFind = FindFirstFileW(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      DeleteFileW((std::wstring(path.c_str()) + fd.cFileName).c_str());
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
  }
}

void Server::makePlayerAKing(const User &user) {
  _kings.add(user.name());
  this->broadcastToArea(user.location(), SV_KING, user.name());
}

void Server::killAllObjectsOwnedBy(const Permissions::Owner &owner) {
  const auto &serials = _objectsByOwner.getObjectsWithSpecificOwner(owner);
  for (auto serial : serials) {
    auto *object = _entities.find<Object>(serial);
    object->reduceHealth(object->health());
  }
}

void Server::publishStats(const Server *server) {
  ++server->_threadsOpen;

  auto statsFile = std::ofstream{"logging/stats.js"};
  statsFile << "stats = {\n\n";

  statsFile << "version: \"" << version() << "\",\n";

  statsFile << "time: " << server->_time << ",\n";

  statsFile << "recipes: " << server->_recipes.size() << ",\n";
  statsFile << "constructions: " << server->_numBuildableObjects << ",\n";

  statsFile << "users: [";
  for (const auto userEntry : server->_usersByName) {
    const auto &user = *userEntry.second;
    statsFile << "\n{"
              << "name: \"" << user.name() << "\","
              << "class: \"" << user.getClass().type().id() << "\","
              << "level: \"" << user.level() << "\","
              << "xp: \"" << user.xp() << "\","
              << "xpNeeded: \"" << user.XP_PER_LEVEL[user.level()] << "\","
              << "x: \"" << user.location().x << "\","
              << "y: \"" << user.location().y << "\","
              << "city: \"" << server->_cities.getPlayerCity(user.name())
              << "\","
              << "isKing: " << server->_kings.isPlayerAKing(user.name()) << ","
              << "health: " << user.health() << ","
              << "maxHealth: " << user.stats().maxHealth << ","
              << "energy: " << user.energy() << ","
              << "maxEnergy: " << user.stats().maxEnergy << ","
              << "knownRecipes: " << user.knownRecipes().size() << ","
              << "knownConstructions: " << user.knownConstructions().size()
              << ",";

    statsFile << "inventory: [";
    for (auto inventorySlot : user.inventory()) {
      auto id =
          inventorySlot.first != nullptr ? inventorySlot.first->id() : ""s;
      statsFile << "{id:\"" << id << "\", qty:" << inventorySlot.second << "},";
    }
    statsFile << "],";

    statsFile << "gear: [";
    for (auto gearSlot : user.gear()) {
      auto id = gearSlot.first != nullptr ? gearSlot.first->id() : ""s;
      statsFile << "{id:\"" << id << "\", qty:" << gearSlot.second << "},";
    }
    statsFile << "],";

    statsFile << "},\n";
  }
  statsFile << "],\n";

  statsFile << "\n};\n";

  --server->_threadsOpen;
}

void Server::initialiseData() {
  // Connect ranged weapons with their ammo
  for (auto &itemConst : _items) {
    auto &item = const_cast<ServerItem &>(itemConst);
    item.fetchAmmoItem();
  }

  ProgressLock::registerStagedLocks();

  // Remove invalid items referred to by objects/recipes
  for (auto it = _items.begin(); it != _items.end();) {
    if (!it->valid()) {
      _items.erase(it++);
    } else
      ++it;
  }
}
