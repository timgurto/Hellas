#include <iostream>
#include <sstream>

#ifndef SINGLE_THREAD
#include <mutex>
#include <thread>
#endif

#include "../Socket.h"
#include "../messageCodes.h"
#include "../threadNaming.h"
#include "../util.h"
#include "../versionUtil.h"
#include "Groups.h"
#include "ItemSet.h"
#include "NPC.h"
#include "NPCType.h"
#include "ProgressLock.h"
#include "SRecipe.h"
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

const ms_t Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 1000;

const px_t Server::ACTION_DISTANCE = Podes{4}.toPixels();
const px_t Server::CULL_DISTANCE = 450;

Server::Server()
    : _time(SDL_GetTicks()),
      _lastTime(_time),
      _socket(),
      _debug("server.log"),
      _userFilesPath("Users/"),
      _lastSave(_time),
      _timeStatsLastPublished(_time),
      groups(new Groups) {
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
  if (!_socket.isBound()) return;
  /*_debug << "Server address: " << inet_ntoa(serverAddr.sin_addr) << ":"
         << ntohs(serverAddr.sin_port) << Log::endl;*/
  _socket.listen();
}

Server::~Server() {
  saveData(_entities, _wars, _cities);
  for (auto pair : _terrainTypes) delete pair.second;
  for (const auto &spellPair : _spells) delete spellPair.second;
  ProgressLock::cleanup();

  delete groups;

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
    _debug << Color::CHAT_ERROR
           << "Error polling sockets: " << WSAGetLastError() << Log::endl;
    return;
  }
  _time = SDL_GetTicks();

  // Activity on server socket: new connection
  if (FD_ISSET(_socket.getRaw(), &readFDs)) {
    if (false && _clientSockets.size() == MAX_CLIENTS) {
      _debug("No room for additional clients; all slots full");
      sockaddr_in clientAddr;
      SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr *)&clientAddr,
                                 (int *)&Socket::sockAddrSize);
      Socket s(tempSocket, {});
      // Allow time for rejection message to be sent before closing socket
      s.delayClosing(5000);
      sendMessage(s, WARNING_SERVER_FULL);
    } else {
      sockaddr_in clientAddr;
      SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr *)&clientAddr,
                                 (int *)&Socket::sockAddrSize);
      if (tempSocket == SOCKET_ERROR) {
        _debug << Color::CHAT_ERROR
               << "Error accepting connection: " << WSAGetLastError()
               << Log::endl;
      } else {
        auto ip = std::string{inet_ntoa(clientAddr.sin_addr)};
        _debug << Color::CHAT_SUCCESS << "Connection accepted: " << ip << ":"
               << ntohs(clientAddr.sin_port)
               << ", socket number = " << tempSocket << Log::endl;
        _clientSockets.insert({tempSocket, ip});
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
        removeUser({raw, {}});
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
  if (!_socket.isBound()) return;

  if (!_dataLoaded) DataLoader::FromPath(*this).load();
  initialiseData();
  publishGameData();

  loadWorldState();
  if (!cmdLineArgs.contains("nospawn")) spawnInitialObjects();

  auto threadsOpen = 0;

  logNumberOfOnlineUsers();
  _onlineAndOfflineUsers.includeUsersFromDataFiles();

  _loop = true;
  _running = true;
  _debug("Server is ready", Color::CHAT_SUCCESS);
  while (_loop) {
    _time = SDL_GetTicks();
    const ms_t timeElapsed = _time - _lastTime;
    _lastTime = _time;

#ifndef _DEBUG
    // Check that clients are alive
    for (std::set<User>::iterator it = _onlineUsers.begin();
         it != _onlineUsers.end();) {
      if (it->hasExceededTimeout()) {
        _debug << Color::CHAT_ERROR << "User " << it->name()
               << " has timed out." << Log::endl;
        std::set<User>::iterator next = it;
        ++next;

        auto socketIt = _clientSockets.find(it->socket());
        if (socketIt == _clientSockets.end()) {
          SERVER_ERROR(
              "Trying to clean up user when socket number doesn't exist");
          ++it;
          continue;
        }
        _clientSockets.erase(socketIt);

        removeUser(it);
        it = next;
      } else {
        ++it;
      }
    }
#endif

    // Save data
    if (_time - _lastSave >= SAVE_FREQUENCY) {
      for (const User &user : _onlineUsers) {
        writeUserData(user);
      }

      std::thread([this]() {
        setThreadName("Saving data on regular timer");
        saveData(_entities, _wars, _cities);
      }).detach();

      _lastSave = _time;
    }

    // Publish stats
    if (!_isTestServer)
      if (_time - _timeStatsLastPublished >= PUBLISH_STATS_FREQUENCY) {
        std::thread([this]() {
          setThreadName("Publishing server stats");
          publishStats();
        }).detach();

        _timeStatsLastPublished = _time;
      }

    // Update users
    for (const User &user : _onlineUsers)
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

    _cities.update(timeElapsed);

    // Deal with any messages from the server
    while (!_messages.empty()) {
      handleBufferedMessages(_messages.front().first, _messages.front().second);
      _messages.pop();
    }

    checkSockets();

    SDL_Delay(1);
  }

  // Save all user data
  for (const User &user : _onlineUsers) {
    writeUserData(user);
  }

  while (_threadsOpen > 0)
    ;
  _running = false;
}

void Server::addUser(const Socket &socket, const std::string &name,
                     const std::string &pwHash, const std::string &classID) {
  auto newUserToInsert = User{name, {}, &socket};

  // Add new user to list
  logNumberOfOnlineUsers();
  std::set<User>::const_iterator it =
      _onlineUsers.insert(newUserToInsert).first;
  auto &newUser = const_cast<User &>(*it);
  _onlineUsersByName[name] = &*it;
  logNumberOfOnlineUsers();

  newUser.pwHash(pwHash);

  // Announce to all
  broadcast({SV_USER_CONNECTED, name});

  // Announce others to him
  sendOnlineUsersTo(newUser);

  newUser.initialiseInventoryAndGear();

  const bool userExisted = readUserData(newUser);
  const auto isNewUser = !userExisted;
  if (isNewUser) {
    _onlineAndOfflineUsers.includeUser(name);
    newUser.setClass(_classes[classID]);
    newUser.moveToSpawnPoint(true);
    _debug << "New";
  } else {
    _debug << "Existing";
    newUser.updateStats();
  }
  _debug << " user, " << name << " has logged in." << Log::endl;

  if (!_isTestServer) newUser.findRealWorldLocation();

  sendMessage(socket, SV_WELCOME);
  if (userExisted) newUser.sendTimePlayed();

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
  newUser.accountForOwnedEntities();

  // Tell him if he's in the tutorial
  if (newUser.isInTutorial()) newUser.sendMessage({SV_YOU_ARE_IN_THE_TUTORIAL});

  // Send him his group
  if (!isNewUser) {
    auto g = groups->getUsersGroup(name);
    groups->sendGroupMakeupTo(g, newUser);
  }

  // Send him his inventory
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    if (newUser.inventory(i).hasItem())
      sendInventoryMessage(newUser, i, Serial::Inventory());
  }

  // Send him his gear
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    if (newUser.gear(i).hasItem())
      sendInventoryMessage(newUser, i, Serial::Gear());
  }

  newUser.sendKnownRecipes();

  // Send him the constructions he knows
  if (newUser.knownConstructions().size() > 0) {
    std::string args = makeArgs(newUser.knownConstructions().size());
    for (const std::string &id : newUser.knownConstructions()) {
      args = makeArgs(args, id);
    }
    newUser.sendMessage({SV_YOUR_CONSTRUCTIONS, args});
  }

  // Send him his talents
  const auto &userClass = newUser.getClass();
  auto treesToSend = std::set<std::string>{};
  for (auto pair : userClass.talentRanks()) {
    const auto &talent = *pair.first;
    newUser.sendMessage({SV_TALENT_INFO, makeArgs(talent.name(), pair.second)});
    treesToSend.insert(talent.tree());
  }
  for (const auto &tree : treesToSend) {
    auto pointsInTree = userClass.pointsInTree(tree);
    newUser.sendMessage({SV_POINTS_IN_TREE, makeArgs(tree, pointsInTree)});
  }

  // Send him his quest progress
  for (const auto &pair : newUser.questsInProgress()) {
    const auto &questID = pair.first;
    const auto *quest = findQuest(questID);
    if (quest->objectives.empty())
      newUser.sendMessage({SV_QUEST_CAN_BE_FINISHED, questID});
    else {
      bool progressHasBeenMade = false;
      for (auto i = 0; i != quest->objectives.size(); ++i) {
        auto &objective = quest->objectives[i];
        auto progress =
            newUser.questProgress(questID, objective.type, objective.id);
        if (progress == 0) continue;
        newUser.sendMessage(
            {SV_QUEST_PROGRESS, makeArgs(questID, i, progress)});
        progressHasBeenMade = true;
      }

      if (!progressHasBeenMade)
        newUser.sendMessage({SV_QUEST_IN_PROGRESS, questID});
    }
  }

  // Teach free spell if a new user, or a returning user without the spell
  auto spellTaught = newUser.getClass().teachFreeSpellIfAny();
  if (!spellTaught.empty())
    newUser.setHotbarAction(1, HOTBAR_SPELL, spellTaught);

  // Send him his known spells
  auto knownSpellsString = userClass.generateKnownSpellsString();
  newUser.sendMessage({SV_KNOWN_SPELLS, knownSpellsString});

  // Other info
  newUser.sendHotbarMessage();
  if (userExisted) newUser.exploration.sendWholeMap(socket);
  _cities.sendInfoAboutCitiesTo(newUser);
  newUser.sendSpawnPoint();

  // Give him starting buffs if he's a new user
  if (isNewUser) {
    for (auto &pair : _buffTypes) {
      const auto &buff = pair.second;
      if (buff.shouldGiveToNewPlayers()) newUser.applyBuff(buff, newUser);
    }
  }

  if (_cities.isPlayerInACity(name)) {
    const auto playerCity = _cities.getPlayerCity(name);
    giveWarDeclarationDebuffsToCitizenAfterTheFact(newUser);
  }

  // Add user to location-indexed trees
  getCollisionChunk(newUser.location()).addEntity(&newUser);
  _usersByX.insert(&newUser);
  _usersByY.insert(&newUser);
  _entitiesByX.insert(&newUser);
  _entitiesByY.insert(&newUser);

  newUser.sendMessage({SV_LOGIN_INFO_HAS_FINISHED});
}

void Server::removeUser(const std::set<User>::iterator &it) {
  const auto &userToDelete = *it;

  // Alert all users
  for (const User &user : _onlineUsers) {
    if (&user == &userToDelete) continue;
    sendMessage(user.socket(), {SV_USER_DISCONNECTED, userToDelete.name()});
  }

  forceAllToUntarget(userToDelete);

  // Save user data
  writeUserData(userToDelete);

  getCollisionChunk(userToDelete.location())
      .removeEntity(userToDelete.serial());
  _usersByX.erase(&userToDelete);
  _usersByY.erase(&userToDelete);
  _entitiesByX.erase(&userToDelete);
  _entitiesByY.erase(&userToDelete);

  logNumberOfOnlineUsers();
  _onlineUsersByName.erase(it->name());
  _onlineUsers.erase(it);
  logNumberOfOnlineUsers();
}

void Server::removeUser(const Socket &socket) {
  const std::set<User>::iterator it = _onlineUsers.find(socket);
  if (it != _onlineUsers.end()) {
    _debug << "Removing user " << it->name() << Log::endl;
    removeUser(it);
  } else
    _debug("User was already removed", Color::CHAT_ERROR);
}

std::set<User *> Server::findUsersInArea(MapPoint loc,
                                         double squareRadius) const {
  static User lowDummy{MapPoint{}}, highDummy{MapPoint{}};
  std::set<User *> users;
  lowDummy.changeDummyLocation(loc.x - squareRadius);
  auto loX = _usersByX.lower_bound(&lowDummy);
  highDummy.changeDummyLocation(loc.x + squareRadius);
  auto hiX = _usersByX.upper_bound(&highDummy);
  for (auto it = loX; it != hiX && it != _usersByX.end(); ++it)
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

Server::ContainerInfo Server::getContainer(User &user, Serial serial) {
  if (serial.isInventory())
    return {&user.inventory()};

  else if (serial.isGear())
    return {&user.gear()};

  auto ret = ContainerInfo{};
  ret.object = _entities.find<Object>(serial);

  if (!isEntityInRange(user.socket(), user, ret.object)) return WARNING_TOO_FAR;
  if (!ret.object->permissions.canUserAccessContainer(user.name()))
    return WARNING_NO_PERMISSION;

  if (ret.object->hasContainer())
    ret.container = &ret.object->container().raw();
  return ret;
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
  if (distance(user, *ent) > ACTION_DISTANCE) {
    if (!suppressErrorMessages) sendMessage(client, WARNING_TOO_FAR);
    return false;
  }

  return true;
}

void Server::forceAllToUntarget(const Entity &target,
                                const User *userToExclude) {
  // Fix users targeting the entity
  auto serial = target.serial();
  for (const User &constUser : _onlineUsers) {
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

    // Fix entities tagged by the entity
    if (entity.tagger == target) entity.tagger.onDisconnect();
  }
}

void Server::removeEntity(Entity &ent, const User *userToExclude) {
  // Ensure no other users are targeting this object, as it will be removed.
  forceAllToUntarget(ent, userToExclude);

  // Alert nearby users of the removal
  auto serial = ent.serial();
  for (const User *userP : findUsersInArea(ent.location()))
    userP->sendMessage({SV_OBJECT_REMOVED, serial});

  getCollisionChunk(ent.location()).removeEntity(serial);
  _entitiesByX.erase(&ent);
  _entitiesByY.erase(&ent);
  auto numRemoved = _entities.erase(&ent);
  delete &ent;
  if (numRemoved != 1) {
    SERVER_ERROR("Removed the wrong number of entities");
  }
}

void Server::gatherObject(Serial serial, User &user) {
  // Give item to user
  auto *ent = _entities.find(serial);
  const ServerItem *const toGive = ent->gatherable.chooseRandomItem();
  size_t qtyToRemove = ent->gatherable.chooseRandomQuantity(toGive);
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
  ent->gatherable.removeItem(toGive, qtyToRemove);
  if (!ent->gatherable.hasItems()) {
    if (ent->type()->transformation.mustBeGathered) {
      forceAllToUntarget(*ent);
      ent->gatherable.removeAllGatheringUsers();
    } else
      removeEntity(*ent, &user);
  } else
    ent->gatherable.decrementGatheringUsers();

#ifdef SINGLE_THREAD
  saveData(_objects, _wars, _cities);
#else
  std::thread([this]() {
    setThreadName("Saving data after gather");
    saveData(_entities, _wars, _cities);
  }).detach();
#endif
}

void Server::removeAllObjectsOwnedBy(const Permissions::Owner &owner) {
  auto serials = _objectsByOwner.getObjectsWithSpecificOwner(owner);
  for (auto serial : serials) {
    auto &entity = *this->_entities.find(serial);
    this->removeEntity(entity);
  }
}

void Server::spawnInitialObjects() {
  // From spawners
  auto timeOfLastReport = SDL_GetTicks();

  auto numSpawners = _spawners.size();
  auto i = 0;
  for (auto &spawner : _spawners) {
    if (!spawner.type()) {
      SERVER_ERROR("Spawner has no type");
      return;
    }
    for (size_t i = 0; i != spawner.quantity(); ++i) spawner.spawn();
    ++i;

    const auto REPORTING_TIME = 500;
    auto currentTime = SDL_GetTicks();
    if (currentTime - timeOfLastReport >= REPORTING_TIME) {
      timeOfLastReport = currentTime;
      _debug << Color::CHAT_DEFAULT << "Loading spawners: " << i << "/"
             << numSpawners << " (" << 100 * i / numSpawners << "%)"
             << Log::endl;
    }
  }
}

bool Server::itemIsTag(const ServerItem *item,
                       const std::string &tagName) const {
  if (!item) {
    SERVER_ERROR("Checking tags of null item");
    return false;
  }
  return item->isTag(tagName);
}

ObjectType *Server::findObjectTypeByID(const std::string &id) const {
  for (auto *type : _objectTypes)
    if (type->id() == id) return const_cast<ObjectType *>(type);
  return nullptr;
}

Object &Server::addObject(const ObjectType *type, const MapPoint &location,
                          const Permissions::Owner &owner) {
  Object *newObj =
      type->classTag() == 'v'
          ? new Vehicle(dynamic_cast<const VehicleType *>(type), location)
          : new Object(type, location);

  switch (owner.type) {
    case Permissions::Owner::ALL_HAVE_ACCESS:
      break;

    case Permissions::Owner::NO_ACCESS:
      newObj->permissions.setNoAccess();
      break;

    case Permissions::Owner::PLAYER:
      newObj->permissions.setPlayerOwner(owner.name);
      _objectsByOwner.add(owner, newObj->serial());
      {
        const auto *user = this->getUserByName(owner.name);
        if (user != nullptr) user->registerObjectIfPlayerUnique(*type);
      }

      break;
    case Permissions::Owner::CITY:
      newObj->permissions.setCityOwner(owner.name);
      break;
  }

  if (!type->container().spawnsWithItem.empty()) {
    const auto *item = findItem(type->container().spawnsWithItem);
    if (item) newObj->container().addItems(item);
  }

  return dynamic_cast<Object &>(addEntity(newObj));
}

Object &Server::addPermanentObject(const ObjectType *type,
                                   const MapPoint &location) {
  Object *newObj = new Object(type, location);
  newObj->setAsPermanent();
  newObj->excludeFromPersistentState();

  return dynamic_cast<Object &>(addEntity(newObj));
}

NPC &Server::addNPC(const NPCType *type, const MapPoint &location) {
  NPC *newNPC = new NPC(type, location);
  newNPC->permissions.setAsMob();
  addEntity(newNPC);
  return dynamic_cast<NPC &>(*newNPC);
}

Entity &Server::addEntity(Entity *newEntity) {
  _entities.insert(newEntity);
  const MapPoint &loc = newEntity->location();

  // Alert nearby users
  if (newEntity->shouldBePropagatedToClients()) {
    auto isNew = _running;  // i.e., not loading
    for (const User *userP : findUsersInArea(loc))
      newEntity->sendInfoToClient(*userP, isNew);
  }

  // Add entity to relevant chunk
  if (newEntity->type()->collides())
    getCollisionChunk(loc).addEntity(newEntity);

  // Add entity to x/y index sets
  _entitiesByX.insert(newEntity);
  _entitiesByY.insert(newEntity);

  return const_cast<Entity &>(*newEntity);
}

void Server::giveWarDeclarationDebuffs(const Belligerent declarer) {
  auto usersToDebuff = std::set<std::string>{};

  if (declarer.type == Belligerent::PLAYER)
    usersToDebuff.insert(declarer.name);
  else {
    usersToDebuff = cities().membersOf(declarer.name);
    _cities.onCityDeclaredWar(declarer.name);
  }

  for (const auto &username : usersToDebuff) {
    auto *userToDebuff = getUserByName(username);
    if (!userToDebuff) return;

    for (const auto *buffType : _warDeclarationDebuffs) {
      userToDebuff->applyDebuff(*buffType, *userToDebuff);
    }
  }
}

void Server::giveWarDeclarationDebuffsToCitizenAfterTheFact(User &user) {
  const auto cityName = _cities.getPlayerCity(user.name());

  for (const auto *buffType : _warDeclarationDebuffs) {
    const auto buffDuration =
        _cities.getRemainingTimeOnWarDebuff(cityName, *buffType);
    if (buffDuration > 0) user.applyDebuff(*buffType, user, buffDuration);
  }
}

User *Server::getUserByName(const std::string &username) {
  if (_onlineUsersByName.empty()) return nullptr;
  auto it = _onlineUsersByName.find(username);
  if (it == _onlineUsersByName.end()) return nullptr;
  return const_cast<User *>(it->second);
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

const ServerItem *Server::createAndFindItem(const std::string &id) {
  auto it = _items.insert(ServerItem{id}).first;
  return &*it;
}

const BuffType *Server::findBuff(const BuffType::ID &id) const {
  auto it = _buffTypes.find(id);
  if (it == _buffTypes.end()) return nullptr;
  return &it->second;
}

const Spell *Server::findSpell(const Spell::ID &id) const {
  auto it = _spells.find(id);
  if (it == _spells.end()) return nullptr;
  return it->second;
}

std::pair<std::set<Serial>::iterator, std::set<Serial>::iterator>
Server::findObjectsOwnedBy(const Permissions::Owner &owner) const {
  return _objectsByOwner.getObjectsOwnedBy(owner);
}

Entity *Server::findEntityBySerial(Serial serial) {
  return _entities.find(serial);
}

const MapRect *Server::findNPCTemplate(const std::string &templateID) const {
  auto it = _npcTemplates.find(templateID);
  if (it == _npcTemplates.end()) return nullptr;
  return &it->second;
}

const Recipe *Server::findRecipe(const std::string &recipeID) const {
  auto it = _recipes.find(recipeID);
  if (it == _recipes.end()) return nullptr;
  return &*it;
}

const Terrain *Server::terrainType(char index) const {
  auto it = _terrainTypes.find(index);
  if (it == _terrainTypes.end()) return nullptr;
  return it->second;
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

bool Server::doesPlayerExist(std::string username) const {
  return _onlineAndOfflineUsers.includes(username);
}

void Server::makePlayerAKing(const User &user) {
  _kings.add(user.name());
  this->broadcastToArea(user.location(), {SV_KING, user.name()});
}

void Server::MoveAllObjectsFromOwnerToOwner(
    const Permissions::Owner &oldOwner, const Permissions::Owner &newOwner) {
  const auto serials = _objectsByOwner.getObjectsWithSpecificOwner(oldOwner);
  for (auto serial : serials) {
    auto *object = _entities.find<Object>(serial);
    object->permissions.setOwner(newOwner);
  }
}

void Server::addObjectType(const ObjectType *p) { _objectTypes.insert(p); }

void Server::initialiseData() {
  // Connect ranged weapons with their ammo
  for (auto &itemConst : _items) {
    auto &item = const_cast<ServerItem &>(itemConst);
    item.fetchAmmoItem();
  }

  ProgressLock::registerStagedLocks();

  // Remove invalid items referred to by objects/recipes
  for (const auto &item : _items)
    if (!item.valid())
      SERVER_ERROR("Item referred to but not specified: "s + item.id());

  for (auto &ot : _objectTypes) ot->initialise();
}

void Server::OnlineAndOfflineUsers::includeUsersFromDataFiles() {
  for (const auto &pair : getUsersFromFiles()) includeUser(pair.first);
}

bool Server::OnlineAndOfflineUsers::includes(std::string username) const {
  return _container.count(username) == 1;
}
