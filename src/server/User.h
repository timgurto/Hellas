#ifndef USER_H
#define USER_H

#include <windows.h>

#include <iostream>
#include <string>

#include "../Optional.h"
#include "../Point.h"
#include "../Socket.h"
#include "../Stats.h"
#include "City.h"
#include "Class.h"
#include "Entity.h"
#include "Exploration.h"
#include "SRecipe.h"
#include "ServerItem.h"
#include "objects/Object.h"

class NPC;
class Server;

// Stores information about a single user account for the server
class User : public Object {  // TODO: Don't inherit from Object
 public:
  enum Action {
    GATHER,
    CRAFT,
    CONSTRUCT,
    DECONSTRUCT,
    ATTACK,

    NO_ACTION
  };

 private:
  std::string _name;
  std::string _pwHash;
  Optional<Socket> _socket;
  std::string _realWorldLocation;

  Optional<Class> _class;

  Action _action;
  ms_t _actionTime;  // Time remaining on current action.
  // Information used when action completes:
  Object *_actionObject;                // Gather, deconstruct
  const SRecipe *_actionRecipe;         // Craft
  const ObjectType *_actionObjectType;  // Construct
  size_t _actionSlot;                   // Construct
  MapPoint _actionLocation;             // Construct
  bool _actionOwnedByCity;              // Construct

  Exploration _exploration;

  std::set<std::string> _knownRecipes, _knownConstructions;
  mutable std::set<std::string> _playerUniqueCategoriesOwned;

  size_t _driving;  // The serial of the vehicle this user is currently driving;
                    // 0 if none.

  ServerItem::vect_t _inventory, _gear;

  struct HotbarAction {
    HotbarCategory category{HOTBAR_NONE};
    std::string id{};
    operator bool() const { return category != HOTBAR_NONE; }
  };
  std::vector<HotbarAction> _hotbar;

  bool _shouldSuppressAmmoWarnings{false};  // To prevent spam

  ms_t _lastContact;
  ms_t _latency;

  int _secondsPlayedBeforeThisSession{0};
  Uint32 _serverTicksAtLogin{SDL_GetTicks()};

  MapPoint _respawnPoint;

  bool _isInitialised{false};

  XP _xp = 0;
#ifdef _DEBUG
  Level _level = 1;
#else
  Level _level = 1;
#endif
  void sendXPMessage() const;
  void announceLevelUp() const;

  std::map<Quest::ID, ms_t> _quests;  // second: time remaining of time limit
  struct QuestProgress {
    Quest::ID quest;
    Quest::Objective::Type type;
    std::string ID;
    bool operator<(const QuestProgress &rhs) const;
  };
  std::map<QuestProgress, int> _questProgress;
  std::set<Quest::ID> _questsCompleted;

  bool _isInCombat{false};

 public:
  User(const std::string &name, const MapPoint &loc, const Socket *socket);
  User(const Socket &rhs);  // for use with set::find(), allowing find-by-socket
  User(const MapPoint
           &loc);  // for use with set::find(), allowing find-by-location
  virtual ~User() {}

  bool operator<(const User &rhs) const {
    return _socket.value() < rhs._socket.value();
  }

  const std::string &name() const { return _name; }
  void pwHash(const std::string &hash) { _pwHash = hash; }
  const std::string &pwHash() const { return _pwHash; }
  const Socket &socket() const { return _socket.value(); }
  bool hasSocket() const { return _socket.hasValue(); }
  void findRealWorldLocation();
  static void findRealWorldLocationStatic(User *user);
  void setRealWorldLocation(const std::string &location) {
    _realWorldLocation = location;
  }
  const std::string &realWorldLocation() const;
  void secondsPlayedBeforeThisSession(int t) {
    _secondsPlayedBeforeThisSession = t;
  };
  int secondsPlayedThisSession() const;
  int secondsPlayed() const;
  mutable int secondsOffline{0};  // Used only for logging; 0 for online players
  void sendTimePlayed() const;
  Message teleportMessage(const MapPoint &destination) const override;
  void onTeleport() override;
  const Class &getClass() const { return _class.value(); }
  Class &getClass() { return _class.value(); }
  void setClass(const ClassType &type) { _class = {type, *this}; }
  size_t driving() const { return _driving; }
  void driving(size_t serial) { _driving = serial; }
  bool isDriving() const { return _driving != 0; }
  const std::set<std::string> &knownRecipes() const { return _knownRecipes; }
  void addRecipe(const std::string &id) { _knownRecipes.insert(id); }
  bool knowsRecipe(const std::string &id) const;
  const std::set<std::string> &knownConstructions() const {
    return _knownConstructions;
  }
  void addConstruction(const std::string &id) {
    _knownConstructions.insert(id);
  }
  void removeConstruction(const std::string &id) {
    _knownConstructions.erase(id);
  }
  bool knowsConstruction(const std::string &id) const;
  bool hasRoomToCraft(const SRecipe &recipe) const;
  bool shouldGatherDoubleThisTime() const;
  bool hasPlayerUnique(const std::string &category) const {
    return _playerUniqueCategoriesOwned.find(category) !=
           _playerUniqueCategoriesOwned.end();
  }
  const MapPoint &respawnPoint() const { return _respawnPoint; }
  void respawnPoint(const MapPoint &loc) { _respawnPoint = loc; }
  const Exploration &exploration() const { return _exploration; }
  Exploration &exploration() { return _exploration; }
  void setHotbarAction(size_t button, int category, const std::string &id);
  void sendHotbarMessage();
  const std::vector<HotbarAction> &hotbar() const { return _hotbar; }

  void onMove() override;

  // Inventory getters/setters
  const ServerItem::Slot &inventory(size_t index) const {
    return _inventory[index];
  }
  ServerItem::Slot &inventory(size_t index) { return _inventory[index]; }
  ServerItem::vect_t &inventory() { return _inventory; }
  const ServerItem::vect_t &inventory() const { return _inventory; }
  bool hasRoomFor(std::set<std::string> itemNames) const;

  // Gear getters/setters
  const ServerItem::Slot &gear(size_t index) const { return _gear[index]; }
  ServerItem::Slot &gear(size_t index) { return _gear[index]; }
  ServerItem::vect_t &gear() { return _gear; }
  const ServerItem::vect_t &gear() const { return _gear; }

  static void init();
  bool isInitialised() const { return _isInitialised; }
  void markAsInitialised() { _isInitialised = true; }

  void updateStats() override;
  ms_t timeToRemainAsCorpse() const override { return 0; }
  bool canBeAttackedBy(const User &user) const override;
  px_t attackRange() const override;
  CombatResult generateHitAgainst(const Entity &target, CombatType type,
                                  SpellSchool school,
                                  px_t range) const override;
  void sendGotHitMessageTo(const User &user) const override;
  bool canBlock() const override;
  bool isAttackingTarget() const override { return _action == ATTACK; }
  SpellSchool school() const override;
  Level level() const override { return _level; }
  double combatDamage() const override;
  bool isInCombat() const { return _isInCombat; }
  void putInCombat() { _isInCombat = true; }

  char classTag() const override { return 'u'; }
  void loadBuff(const BuffType &type, ms_t timeRemaining) override;
  void loadDebuff(const BuffType &type, ms_t timeRemaining) override;
  void sendBuffMsg(const Buff::ID &buff) const override;
  void sendDebuffMsg(const Buff::ID &buff) const override;
  void sendLostBuffMsg(const Buff::ID &buff) const override;
  void sendLostDebuffMsg(const Buff::ID &buff) const override;

  void onHealthChange() override;
  void onEnergyChange() override;
  void onDeath() override;
  void onNewOwnedObject(const ObjectType &type) const;
  void onDestroyedOwnedObject(const ObjectType &type) const;
  void onAttackedBy(Entity &attacker, Threat threat) override;
  void onKilled(Entity &victim) override;
  bool canAttack() override;
  void onCanAttack() override;
  void onAttack() override;
  void onSpellcast(const Spell::ID &id, const Spell &spell);
  void sendRangedHitMessageTo(const User &userToInform) const override;
  void sendRangedMissMessageTo(const User &userToInform) const override;
  void broadcastDamagedMessage(Hitpoints amount) const override;
  void broadcastHealedMessage(Hitpoints amount) const override;

  void sendInfoToClient(const User &targetUser) const override;
  void sendInventorySlot(size_t slot) const;
  void sendGearSlot(size_t slot) const;

  void onOutOfRange(const Entity &rhs) const override;
  Message outOfRangeMessage() const override;

  const MapRect collisionRect() const;

  Action action() const { return _action; }
  void action(Action a) { _action = a; }
  const Object *actionObject() const { return _actionObject; }
  void beginGathering(
      Object *object);  // Configure user to perform an action on an object
  void setTargetAndAttack(Entity *target);  // Configure user to prepare to
                                            // attack an NPC or player

  // Whether the user has enough materials to craft a recipe
  bool hasItems(const ItemSet &items) const;
  bool hasItems(const std::string &tag, size_t quantity) const;
  void removeItems(const ItemSet &items);
  void removeItems(const std::string &tag, size_t quantity);
  int countItems(const ServerItem *item) const;
  class ToolSearchResult {
   public:
    enum Type { NOT_FOUND, DAMAGE_ON_USE, TERRAIN };
    ToolSearchResult(DamageOnUse &tool) : _type(DAMAGE_ON_USE), _tool(&tool) {}
    ToolSearchResult(Type type);
    operator bool() const;
    void use() const;

   private:
    Type _type{NOT_FOUND};
    DamageOnUse *_tool{nullptr};
  };
  bool checkAndDamageTools(const std::set<std::string> &tags);
  bool checkAndDamageTool(const std::string &tag);

 private:
  ToolSearchResult findTool(const std::string &tagName);

 public:
  void clearInventory();
  void clearGear();

  void beginCrafting(const SRecipe &item);  // Configure user to craft an item

  // Configure user to construct an item, or an object from no item
  void beginConstructing(const ObjectType &obj, const MapPoint &location,
                         bool cityOwned, size_t slot = INVENTORY_SIZE);

  // Configure user to deconstruct an object
  void beginDeconstructing(Object &obj);

  void cancelAction();  // Cancel any action in progress, and alert the client
  void finishAction();  // An action has just ended; clean up the state.

  std::string makeLocationCommand() const;

  static const size_t INVENTORY_SIZE = 10;
  static const size_t GEAR_SLOTS = 8;

  static ObjectType OBJECT_TYPE;

  void contact();
  bool alive()
      const;  // Whether the client has contacted the server recently enough

  // Return value: 0 if there was room for all items, otherwise the remainder.
  size_t giveItem(const ServerItem *item, size_t quantity = 1);

  static const Level MAX_LEVEL = 60;
  static const std::vector<XP> XP_PER_LEVEL;
  void level(Level l) { _level = l; }
  XP xp() const { return _xp; }
  void xp(XP newXP) { _xp = newXP; }
  void addXP(XP amount);
  void levelUp();
  int getLevelDifference(const User &user) const override {
    return level() - user.level();
  }

  void sendMessage(MessageCode msgCode, const std::string &args = {}) const;

  void update(ms_t timeElapsed);

  static MapPoint newPlayerSpawn, postTutorialSpawn;
  static double spawnRadius;
  void setSpawnPointToPostTutorial() { _respawnPoint = postTutorialSpawn; }
  void moveToSpawnPoint(bool isNewPlayer = false);
  void onTerrainListChange(const std::string &listID);

  // Quests
  void startQuest(const Quest &quest);
  void completeQuest(const Quest::ID &id);
  void giveQuestReward(const Quest::Reward &reward);
  bool hasCompletedQuest(const Quest::ID &id) const;
  bool hasCompletedAllPrerequisiteQuestsOf(const Quest::ID &id) const;
  const std::set<Quest::ID> &questsCompleted() const {
    return _questsCompleted;
  }
  void abandonQuest(Quest::ID id);
  const std::map<Quest::ID, ms_t> &questsInProgress() const { return _quests; }
  void markQuestAsCompleted(const Quest::ID &id);
  void markQuestAsStarted(const Quest::ID &id, ms_t timeRemaining);
  void addQuestProgress(Quest::Objective::Type type, const std::string &id);
  void initQuestProgress(const Quest::ID &questID, Quest::Objective::Type type,
                         const std::string &id, int qty);
  int questProgress(const Quest::ID &quest, Quest::Objective::Type type,
                    const std::string &id) const;
  int numQuests() const { return _quests.size(); }
  bool isOnQuest(const Quest::ID &id) const {
    return _quests.find(id) != _quests.end();
  }
  bool canStartQuest(const Quest::ID &quest) const;

  struct compareXThenSocketThenAddress {
    bool operator()(const User *a, const User *b) const;
  };
  struct compareYThenSocketThenAddress {
    bool operator()(const User *a, const User *b) const;
  };
  typedef std::set<const User *, User::compareXThenSocketThenAddress> byX_t;
  typedef std::set<const User *, User::compareYThenSocketThenAddress> byY_t;
};

#endif
