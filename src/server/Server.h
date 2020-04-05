#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <queue>
#include <set>
#include <string>
#include <utility>

#include "../Args.h"
#include "../Map.h"
#include "../Socket.h"
#include "../Terrain.h"
#include "../TerrainList.h"
#include "../messageCodes.h"
#include "Buff.h"
#include "City.h"
#include "Class.h"
#include "CollisionChunk.h"
#include "DataLoader.h"
#include "Entities.h"
#include "ItemSet.h"
#include "LogConsole.h"
#include "NPC.h"
#include "ObjectsByOwner.h"
#include "Quest.h"
#include "SRecipe.h"
#include "ServerItem.h"
#include "Spawner.h"
#include "Spell.h"
#include "User.h"
#include "Wars.h"
#include "objects/Object.h"

class MessageParser;

#define SERVER_ERROR(msg)                                              \
  Server::debug() << Color::CHAT_ERROR << (msg) << Log::endl           \
                  << Color::CHAT_ERROR << __FILE__ << ":"s << __LINE__ \
                  << Log::endl

class Server {
 public:
  Server();
  ~Server();
  void run();

  static const u_short DEBUG_PORT = 8888;
  static const u_short PRODUCTION_PORT = 8889;

  static const ms_t
      CLIENT_TIMEOUT;  // How much radio silence before we drop a client
  static const ms_t MAX_TIME_BETWEEN_LOCATION_UPDATES;

  static const px_t ACTION_DISTANCE;  // How close a character must be to
                                      // interact with an object
  static const px_t CULL_DISTANCE;    // Users only get information within a
                                      // circle with this radius.

  const Map &map() { return _map; }

  static Server &instance() { return *_instance; }
  static bool hasInstance() { return _instance != nullptr; }
  static LogConsole &debug() { return *_debugInstance; }

  bool itemIsTag(const ServerItem *item, const std::string &tagName) const;

  mutable LogConsole _debug;

  bool _isTestServer{false};

  // Const Searches/queries
  char findTile(const MapPoint &p)
      const;  // Find the tile type at the specified location.
  std::pair<size_t, size_t> getTileCoords(const MapPoint &p) const;
  std::set<User *> findUsersInArea(MapPoint loc,
                                   double squareRadius = CULL_DISTANCE) const;
  std::set<Entity *> findEntitiesInArea(
      MapPoint loc, double squareRadius = CULL_DISTANCE) const;
  ObjectType *findObjectTypeByID(const std::string &id) const;  // Linear
  User *getUserByName(const std::string &username);
  const BuffType *getBuffByName(const Buff::ID &id) const;
  const Quest *findQuest(const Quest::ID &id) const;
  const ServerItem *findItem(const std::string &id) const;
  const ServerItem *createAndFindItem(const std::string &id);
  const BuffType *findBuff(const BuffType::ID &id) const;
  const Spell *findSpell(const Spell::ID &id) const;
  std::pair<std::set<Serial>::iterator, std::set<Serial>::iterator>
  findObjectsOwnedBy(const Permissions::Owner &owner) const;
  const Entity *findEntityBySerial(Serial serial);
  const MapRect *findNPCTemplate(const std::string &templateID) const;

  // Checks whether an entity is within range of a user.  If not, a relevant
  // error message is sent to the client.
  bool isEntityInRange(const Socket &client, const User &user,
                       const Entity *ent,
                       bool suppressErrorMessages = false) const;

  const Terrain *terrainType(char index) const;

  // Messages
  std::queue<std::pair<Socket, std::string>> _messages;
  static const size_t BUFFER_SIZE = 1023;
  char _stringInputBuffer[BUFFER_SIZE + 1];
  void sendMessage(const Socket &dstSocket, const Message &msg) const;
  void sendMessageIfOnline(const std::string username,
                           const Message &msg) const;
  void broadcast(const Message &msg);  // Send a command to all users
  void broadcastToArea(const MapPoint &location, const Message &msg) const;
  void broadcastToCity(const std::string &cityName, const Message &msg) const;
  void handleBufferedMessages(const Socket &client, const std::string &msg);
  void sendInventoryMessageInner(const User &user, Serial serial, size_t slot,
                                 const ServerItem::vect_t &itemVect) const;
  void sendInventoryMessage(const User &user, size_t slot,
                            const Object &obj) const;
  void sendInventoryMessage(const User &user, size_t slot, Serial serial) const;
  void sendMerchantSlotMessage(const User &user, const Object &obj,
                               size_t slot) const;
  void sendConstructionMaterialsMessage(const User &user,
                                        const Object &obj) const;
  void sendNewBuildsMessage(const User &user,
                            const std::set<std::string> &ids) const;
  void sendNewRecipesMessage(const User &user,
                             const std::set<std::string> &ids) const;
  void alertUserToWar(const std::string &username,
                      const Belligerent &otherBelligerent,
                      bool isUserCityTheBelligerent) const;
  void sendRelevantEntitiesToUser(const User &user);
  void sendOnlineUsersTo(const User &recipient) const;

  // Getters
  const Cities &cities() const { return _cities; }
  const Wars &wars() const { return _wars; }

  // Action functions
  static bool endTutorial(const Object &obj, User &performer,
                          const std::string &textArg);
  static bool createCityOrTeachCityPort(const Object &obj, User &performer,
                                        const std::string &textArg);
  static bool createCity(const Object &obj, User &performer,
                         const std::string &textArg);
  static bool setRespawnPoint(const Object &obj, User &performer,
                              const std::string &textArg);
  // Callback-action functions
  static void destroyCity(const Object &obj);

  void makePlayerAKing(const User &user);

  void killAllObjectsOwnedBy(const Permissions::Owner &owner);

  void incrementThreadCount() { ++_threadsOpen; }
  void decrementThreadCount() { --_threadsOpen; }

  void addObjectType(const ObjectType *p);

 private:
  static Server *_instance;
  static LogConsole *_debugInstance;

  static const int MAX_CLIENTS = 100;

  ms_t _time, _lastTime;

  Socket _socket;

  bool _loop;
  bool _running;  // True while run() is being executed.

  // Clients
  // All connected sockets, including those without registered users
  std::set<Socket> _clientSockets;
  std::set<User> _users;  // All connected users
  // Pointers to all connected users, ordered by name for faster lookup
  mutable std::map<std::string, const User *> _usersByName;
  std::string _userFilesPath;
  void deleteUserFiles();
  /*
  Add the newly logged-in user
  This happens not once the client connects, but rather when a CL_LOGIN_*
  message is received. The classID argument is used only when a new account is
  created.
  */
  void addUser(const Socket &socket, const std::string &name,
               const std::string &pwHash, const std::string &classID = {});
  void checkSockets();

  // Remove traces of a user who has disconnected.
  void removeUser(const Socket &socket);
  void removeUser(const std::set<User>::iterator &it);

  User::byX_t
      _usersByX;  // This and below are for alerting users in a specific area.
  User::byY_t _usersByY;

  // World state
  Entities _entities;          // All entities except Users
  Entity::byX_t _entitiesByX;  // This and below are for alerting users only to
                               // nearby objects.
  Entity::byY_t _entitiesByY;
  ObjectsByOwner _objectsByOwner;

  Wars _wars;
  Cities _cities;
  Kings _kings;

  void loadWorldState(
      const std::string &path = "Data",
      bool shouldKeepOldData = false);  // Attempt to load data from files.
  void loadEntitiesFromFile(const std::string &path,
                            bool shouldBeExcludedFromPersistentState);
  void initialiseData();
  bool _dataLoaded;  // If false when run() is called, load default data.
  static void saveData(const Entities &entities, const Wars &wars,
                       const Cities &cities);
  void spawnInitialObjects();
  volatile mutable int _threadsOpen{0};
  Map _map;

  // World data
  std::set<ServerItem> _items;
  std::set<SRecipe> _recipes;
  std::map<std::string, LootTable> _standaloneLootTables;
  std::map<std::string, MapRect> _npcTemplates;  // Collision rects
  std::set<const ObjectType *> _objectTypes;
  std::vector<Spawner> _spawners;
  std::map<char, Terrain *> _terrainTypes;
  Spells _spells;
  BuffTypes _buffTypes;
  ClassTypes _classes;
  Tiers _tiers;  // Objects are never accessed via container.  This is just
                 // dynamic-object storage.
  Quests _quests;

  size_t _numBuildableObjects = 0;

  std::list<Entity *> _entitiesToRemove;  // Emptied every tick.
  void forceAllToUntarget(const Entity &target,
                          const User *userToExclude = nullptr);
  void removeEntity(Entity &ent, const User *userToExclude = nullptr);
  void gatherObject(Serial serial, User &user);
  void removeAllObjectsOwnedBy(const Permissions::Owner &owner);

  friend class City;
  friend class DataLoader;
  friend class Entity;
  friend class NPC;
  friend class Object;
  friend class Permissions;
  friend class ProgressLock;
  friend class ServerItem;
  friend class Spawner;
  friend class TestServer;
  friend class User;

  NPC &addNPC(const NPCType *type, const MapPoint &location);
  Object &addObject(const ObjectType *type, const MapPoint &location,
                    const Permissions::Owner &owner);
  Entity &addEntity(Entity *newEntity);

  // Collision detection
  static const px_t COLLISION_CHUNK_SIZE;
  CollisionGrid _collisionGrid;
  CollisionChunk &getCollisionChunk(const MapPoint &p);
  std::list<CollisionChunk *> getCollisionSuperChunk(const MapPoint &p);

 public:
  // thisObject = object to omit from collision detection (usually "this", to
  // avoid self-collision)
  bool isLocationValid(const MapPoint &loc, const Entity &thisEntity);
  bool isLocationValid(const MapRect &rect, const Entity &thisEntity);
  bool isLocationValid(const MapPoint &loc, const EntityType &type);

 private:
  bool isLocationValid(const MapRect &rect, const TerrainList &allowedTerrain,
                       const Entity *thisEntity = nullptr);

 private:
  bool readUserData(User &user,
                    bool allowSideEffects = true);  // true: save data existed
  void writeUserData(const User &user) const;
  static const ms_t SAVE_FREQUENCY = 30000;
  ms_t _lastSave;

  static void publishStats(Server *server);
  void generateDurabilityList();
  static const ms_t PUBLISH_STATS_FREQUENCY = 5000;
  ms_t _timeStatsLastPublished;

  void writeUserToFile(const User &user, std::ostream &file) const;

  template <MessageCode M>
  void handleMessage(const Socket &client, User &user, MessageParser &parser);

  void handle_CL_TAKE_ITEM(User &user, Serial serial, size_t slotNum);
  void handle_CL_REPAIR_ITEM(User &user, Serial serial, size_t slot);
  void handle_CL_REPAIR_OBJECT(User &user, Serial serial);
  void handle_CL_TAME_NPC(User &user, Serial serial);
  void handle_CL_LEAVE_CITY(User &user);
  void handle_CL_CEDE(User &user, Serial serial);
  void handle_CL_GRANT(User &user, Serial serial, std::string username);
  void handle_CL_PERFORM_OBJECT_ACTION(User &user, Serial serial,
                                       const std::string &textArg);
  void handle_CL_TARGET_ENTITY(User &user, Serial serial);
  void handle_CL_TARGET_PLAYER(User &user, const std::string &username);
  void handle_CL_SELECT_ENTITY(User &user, Serial serial);
  void handle_CL_SELECT_PLAYER(User &user, const std::string &username);
  void handle_CL_RECRUIT(User &user, std::string username);
  void handle_CL_SUE_FOR_PEACE(User &user, MessageCode code,
                               const std::string &name);
  void handle_CL_CANCEL_PEACE_OFFER(User &user, MessageCode code,
                                    const std::string &name);
  void handle_CL_ACCEPT_PEACE_OFFER(User &user, MessageCode code,
                                    const std::string &name);
  void handle_CL_TAKE_TALENT(User &user, const Talent::Name &talent);
  void handle_CL_UNLEARN_TALENTS(User &user);
  CombatResult handle_CL_CAST(User &user, const std::string &spellID,
                              bool castingFromItem = false);
  void handle_CL_ACCEPT_QUEST(User &user, const Quest::ID &quest,
                              Serial giverSerial);
  void handle_CL_COMPLETE_QUEST(User &user, const Quest::ID &quest,
                                Serial giverSerial);
  void handle_CL_ABANDON_QUEST(User &user, const Quest::ID &quest);
  void handle_CL_AUTO_CONSTRUCT(User &user, Serial serial);
};

#endif
