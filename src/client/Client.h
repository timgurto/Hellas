#ifndef CLIENT_H
#define CLIENT_H

#include <cassert>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

#include "../Args.h"
#include "../Point.h"
#include "../Rect.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../server/Recipe.h"
#include "../types.h"
#include "Avatar.h"
#include "CQuest.h"
#include "ClientBuff.h"
#include "ClientCombatant.h"
#include "ClientConfig.h"
#include "ClientItem.h"
#include "ClientMerchantSlot.h"
#include "ClientNPC.h"
#include "ClientObject.h"
#include "ClientObjectType.h"
#include "ClientSpell.h"
#include "ClientTerrain.h"
#include "ClientWar.h"
#include "Connection.h"
#include "HelpEntry.h"
#include "Images.h"
#include "LogSDL.h"
#include "ParticleProfile.h"
#include "Projectile.h"
#include "SoundProfile.h"
#include "Sprite.h"
#include "Tag.h"
#include "Target.h"
#include "WordWrapper.h"
#include "ui/ChoiceList.h"
#include "ui/Indicator.h"
#include "ui/ItemSelector.h"
#include "ui/OutlinedLabel.h"
#include "ui/Window.h"

class TextBox;

class Client {
 public:
  Client();
  ~Client();

  static Client &instance() {
    assert(_instance != nullptr);
    return *_instance;
  }
  static bool clientExists() { return _instance != nullptr; }
  operator bool() { return clientExists(); }

  void run();
  void gameLoop();
  void loginScreenLoop();  // Alternative game loop to run()

  static bool isClient;

  TTF_Font *defaultFont() const;

  void showErrorMessage(const std::string &message, Color color) const;

  const Avatar &character() const { return _character; }
  const ScreenPoint &offset() const { return _intOffset; }
  const std::string &username() const { return _username; }
  const Sprite *currentMouseOverEntity() const {
    return _currentMouseOverEntity;
  }
  MapRect playerCollisionRect() const { return _character.collisionRect(); }
  const ClientCombatant *targetAsCombatant() const {
    return _target.combatant();
  }
  const Sprite *targetAsEntity() const { return _target.entity(); }
  bool isDismounting() const { return _isDismounting; }
  void attemptDismount() { _isDismounting = true; }
  const SoundProfile *avatarSounds() const { return _avatarSounds; }
  const SoundProfile *generalSounds() const { return _generalSounds; }
  const std::string &tagName(const std::string &id) const {
    return _tagNames[id];
  }
  const HelpEntries &helpEntries() const { return _helpEntries; }
  const Window &helpWindow() const { return *_helpWindow; }
  XP xp() const { return _xp; }

  bool isAtWarWith(const Avatar &user) const;
  bool isAtWarWith(const std::string &username) const;
  bool isAtWarWithPlayerDirectly(const std::string &username) const {
    return _warsAgainstPlayers.atWarWith(username);
  }
  bool isAtWarWithCityDirectly(const std::string &cityName) const {
    return _warsAgainstCities.atWarWith(cityName);
  }
  bool isCityAtWarWithPlayerDirectly(const std::string &username) const {
    return _cityWarsAgainstPlayers.atWarWith(username);
  }
  bool isCityAtWarWithCityDirectly(const std::string &cityName) const {
    return _cityWarsAgainstCities.atWarWith(cityName);
  }

  const Texture &mapImage() const { return _mapImage; }
  const Texture &shadowImage() const { return _shadowImage; }

  const ClientBuffTypes &buffTypes() const { return _buffTypes; }

  const CQuests &quests() const { return _quests; }

  template <typename T>
  void setTarget(const T &newTarget, bool aggressive = false) {
    _target.setAndAlertServer(newTarget, aggressive);
  }
  void clearTarget() { _target.clear(); }
  void hideTargetMenu() { _target.hideMenu(); }

  const Texture &cursorNormal() const { return _cursorNormal; }
  const Texture &cursorGather() const { return _cursorGather; }
  const Texture &cursorContainer() const { return _cursorContainer; }
  const Texture &cursorAttack() const { return _cursorAttack; }
  const Texture &cursorStartsQuest() const { return _cursorStartsQuest; }
  const Texture &cursorEndsQuest() const { return _cursorEndsQuest; }

  static const px_t ICON_SIZE;
  static const px_t TILE_W, TILE_H;
  static const double MOVEMENT_SPEED;
  static const double VEHICLE_SPEED;
  static const Hitpoints MAX_PLAYER_HEALTH;

  enum SpecialSerial {
    INVENTORY = 0,
    GEAR = 1,
  };
  static const size_t INVENTORY_SIZE;
  static const size_t GEAR_SLOTS;

  static const px_t ACTION_DISTANCE;

  static LogSDL &debug() { return *_debugInstance; }

  typedef std::list<Window *> windows_t;
  typedef std::list<Element *>
      ui_t;  // For the UI, that sits below all windows.

  void addWindow(Window *window);
  void removeWindow(Window *window);  // Linear time
  void showWindowInFront(Window *window);
  static void showWindowInFront(void *window) {
    _instance->showWindowInFront(reinterpret_cast<Window *>(window));
  }
  bool isWindowRegistered(const Window *toFind);

  static const px_t SCREEN_X, SCREEN_Y;

  static const px_t CULL_DISTANCE, CULL_HYSTERESIS_DISTANCE;

  static const int MIXING_CHANNELS;

  static std::vector<std::string> GEAR_SLOT_NAMES;

  void infoWindow(const std::string &text);

  void showHelpTopic(const std::string &topic);

  class ParticlesToAdd {
    const ParticleProfile &_profile{nullptr};
    const MapPoint &_location{};
    size_t _qty{0};
    double _customAltitude{0};

   public:
    ParticlesToAdd(const ParticleProfile &profile, const MapPoint &location)
        : _profile(profile), _location(location) {}
    void discrete() { _qty = _profile.numParticlesDiscrete(); }
    void continuous(double delta) {
      _qty = _profile.numParticlesContinuous(delta);
    }
    void exactNumber(size_t qty) { _qty = qty; }
    void customAltitude(double alt) { _customAltitude = alt; }

    size_t quantity() const { return _qty; }
    const ParticleProfile &profile() const { return _profile; }
    const MapPoint &location() const { return _location; }
    double customAltitude() const { return _customAltitude; }
  };
  void addParticles(const ParticlesToAdd &details);
  void addParticles(const ParticleProfile *profile, const MapPoint &location,
                    size_t qty);
  void addParticles(const ParticleProfile *profile,
                    const MapPoint &location);  // Single hit
  void addParticles(const ParticleProfile *profile, const MapPoint &location,
                    double delta);  // /s
  void addParticles(const std::string &profileName,
                    const MapPoint &location);  // Single hit
  void addParticles(const std::string &profileName, const MapPoint &location,
                    double delta);  // /s
  void addParticlesWithCustomAltitude(double altitude,
                                      const std::string &profileName,
                                      const MapPoint &location,
                                      double delta);  // /s
  void addFloatingCombatText(const std::string &text, const MapPoint &location,
                             Color color);

 private:
  static Client *_instance;
  static LogSDL *_debugInstance;

  Connection _connection;
  void disconnect();

  static std::map<std::string, int> _messageCommands;
  static std::map<int, std::string> _errorMessages;
  static void initializeMessageNames();

  ClientConfig _config;
  WordWrapper _wordWrapper;

  std::string _defaultServerAddress;

  static const ms_t MAX_TICK_LENGTH;
  static const ms_t
      SERVER_TIMEOUT;  // How long the client will wait for a ping reply
  static const ms_t
      PING_FREQUENCY;  // How often to test latency with each client
  // How often to send location updates to server (while moving)
  static const ms_t TIME_BETWEEN_LOCATION_UPDATES;

  Texture _cursorNormal, _cursorGather, _cursorContainer, _cursorAttack,
      _cursorStartsQuest, _cursorEndsQuest;
  const Texture *_currentCursor;

  bool _isDismounting;  // Whether the user is looking for a dismount location.

  static void initializeGearSlotNames();

  // Whether the user has the specified item(s).
  bool playerHasItem(const Item *item, size_t quantity = 1) const;

  static void initializeCraftingWindow(Client &client);
  void initializeCraftingWindow();
  bool _haveMatsFilter, _haveToolsFilter, _tagOr, _matOr;
  static const px_t HEADING_HEIGHT;  // The height of windows' section headings
  static const px_t LINE_GAP;   // The total height occupied by a line and its
                                // surrounding spacing
  const Recipe *_activeRecipe;  // The recipe currently selected, if any
  static void startCrafting();  // Called when the "Craft" button is clicked.
  // Populated at load time, after _items
  std::map<std::string, bool> _tagFilters;
  std::map<const ClientItem *, bool> _matFilters;
  mutable bool _tagFilterSelected,
      _matFilterSelected;  // Whether any filters have been selected
  bool recipeMatchesFilters(const Recipe &recipe) const;
  // Called when filters pane is clicked, or new recipes are unlocked.
  static void populateRecipesList(Element &e);
  // Called when a recipe is selected.
  static void selectRecipe(Element &e, const ScreenPoint &mousePos);
  // Called when new recipes are unlocked.
  void populateFilters();
  // Called when new construction items are unlocked
  void populateBuildList();

  std::set<ClientObject *> _objectsWatched;
  void watchObject(ClientObject &obj);
  void unwatchObject(ClientObject &obj);
  void unwatchOutOfRangeObjects();

  bool outsideCullRange(const MapPoint &loc, px_t hysteresis = 0) const;

  ChoiceList *_recipeList = nullptr;
  Element *_detailsPane = nullptr;
  List *_tagList = nullptr, *_materialsList = nullptr;

  Window *_craftingWindow = nullptr, *_buildWindow = nullptr;
  std::set<std::string> _knownRecipes, _knownConstructions;

  void initializeBuildWindow();
  ChoiceList *_buildList = nullptr;
  static void chooseConstruction(Element &e, const ScreenPoint &mousePos);
  const ClientObjectType *_selectedConstruction;
  bool _multiBuild;

  Window *_inventoryWindow = nullptr;
  void initializeInventoryWindow();
  Texture _constructionFootprint;

  Window *_gearWindow = nullptr;
  static void initializeGearWindow(Client &client);
  void initializeGearWindow();
  Texture _gearWindowBackground;
  void onChangeDragItem() { _gearWindow->forceRefresh(); }

  Window *_questLog{nullptr};
  List *_questList{nullptr};
  void initializeQuestLog();
  void populateQuestLog();

  Window *_mapWindow = nullptr;
  Texture _mapImage;
  Texture _shadowImage;
  Picture *_mapPicture{nullptr};
  Element *_mapPins, *_mapPinOutlines;
  static const px_t MAP_IMAGE_W = 300, MAP_IMAGE_H = 300;
  void initializeMapWindow();
  static void updateMapWindow(Element &);
  void Client::addMapPin(const MapPoint &worldPosition, const Color &color);
  void Client::addOutlinedMapPin(const MapPoint &worldPosition,
                                 const Color &color);
  ScreenRect Client::convertToMapPosition(const MapPoint &worldPosition) const;
  int _zoom{0};
  void resizeMap();

  // Social window
  Window *_socialWindow{nullptr};
  void initializeSocialWindow();
  void cleanupSocialWindow();
  List *_warsList{nullptr};
  Element *_citySection{nullptr};
  void refreshCitySection();
  void populateWarsList();
  Label *_numOnlinePlayersLabel{nullptr};
  List *_onlinePlayersList{nullptr};
  void populateOnlinePlayersList();
  std::set<std::string> _allOnlinePlayers{};

  Window *_helpWindow{nullptr};
  void initializeHelpWindow();
  HelpEntries _helpEntries;

  Window *_classWindow{nullptr};
  Element *_talentTrees{nullptr};
  Label *_levelLabel{nullptr};
  Label *_xpLabel{nullptr};
  Label *_pointsAllocatedLabel{nullptr};
  void initializeClassWindow();
  static void confirmAndUnlearnTalents();
  void populateClassWindow();
  std::set<const ClientSpell *> _knownSpells{};
  std::map<std::string, ms_t> _spellCooldowns{};

  windows_t _windows;

  ui_t _ui;
  void addUI(Element *element);
  Element *_castBar;
  Element *_hotbar{nullptr};
  std::vector<Button *> _hotbarButtons = {10, nullptr};
  std::map<ClientBuffType::ID, ms_t> _buffTimeRemaining{},
      _debuffTimeRemaining{};  // Used for the UI only.
  List *_buffsDisplay{nullptr};
  Element *_targetBuffs{nullptr};
  OutlinedLabel *_lastErrorMessage{nullptr};
  mutable ms_t _errorMessageTimer{0};
  Window *_escapeWindow{nullptr};
  List *_questProgress{nullptr};
  void initUI();
  void initChatLog();
  void initWindows();
  void initCastBar();
  void initMenuBar();
  void addButtonToMenu(Element *menuBar, size_t index, Element *toToggle,
                       const std::string iconFile,
                       const std::string tooltip = "");
  void initPerformanceDisplay();
  void initPlayerPanels();
  void initHotbar();
  void populateHotbar();
  void initBuffsDisplay();
  void refreshBuffsDisplay();
  static Element *assembleBuffEntry(const ClientBuffType &type,
                                    bool isDebuff = false);
  void initTargetBuffs();
  void refreshTargetBuffs();
  void initializeEscapeWindow();
  void updateUI();
  List *_toasts{nullptr};
  void initToasts();
  void updateToasts();
  void populateToastsList();
  void toast(const std::string &icon, const std::string &message);
  void initQuestProgress();
  void refreshQuestProgress();

  // Chat
  Element *_chatContainer;
  List *_chatLog;
  TextBox *_chatTextBox;
  void addChatMessage(const std::string &msg,
                      const Color &color = Color::UI_TEXT);
  static Color SAY_COLOR, WHISPER_COLOR;
  std::string _lastWhisperer;  // The username of the last person to whisper,
                               // for fast replying.

  // Character info
  std::string _username;
  void initializeUsername();
  void setRandomUsername();
  Avatar _character;             // Describes the user's character
  Stats _stats;                  // The user's stats
  std::string _displaySpeed{0};  // Speed for display as podes/s
  bool _serverHasOutOfDateLocationInfo{true};

  // Login screen
  Texture _loginFront, _loginBack;
  ui_t _loginUI;
  void initLoginScreen();
  void drawLoginScreen() const;
  std::list<Particle *> _loginParticles;
  void updateLoginParticles(double delta);
  static void login();
  static void connectToServerStatic();
  Indicator *_serverConnectionIndicator{nullptr};
  static void updateLoginButton(void *);
  Window *_createWindow{nullptr};
  void initCreateWindow();
  static void createAccount();
  static void updateCreateButton(void *);
  static void updateClassDescription();
  std::string _autoClassID =
      {};  // For automated account creation, i.e., in tests

  // These are superficial, and relate only to the cast bar.
  ms_t _actionTimer;   // How long the character has been performing the current
                       // action.
  ms_t _actionLength;  // How long the current action should take.
  std::string _actionMessage;  // A description of the current action.
  void prepareAction(const std::string &msg);  // Set up the action, awaiting
                                               // server confirmation.
  void startAction(
      ms_t actionLength);  // Start the action timer.  If zero, stop the timer.

  Target _target;
  /*
  Whether the player is targeting aggressively, i.e., will attack when in range.
  Used here to decide when to alert server of new targets.
  */
  Texture _basePassive, _baseAggressive;
  bool _rightMouseDownWasOnUI;

  bool _loop;
  bool _running;  // True while run() is being executed.
  bool _freeze;   // For testing purposes only; should otherwise remain false.
  bool _shouldAutoLogIn{false};  // Used for tests
  static void exitGame(void *client);

  TTF_Font *_defaultFont;

  // Mouse stuff
  ScreenPoint _mouse;  // Mouse position
  bool _mouseMoved;
  Sprite *getEntityAtMouse();
  void
  checkMouseOver();  // Set state based on window/entity/etc. being moused over.
  bool _mouseOverWindow;  // Whether the mouse is over any visible window.

  bool _leftMouseDown;  // Whether the left mouse button is pressed
  Sprite *_leftMouseDownEntity;
  friend void ClientObject::onLeftClick(Client &client);

  bool _rightMouseDown;
  Sprite *_rightMouseDownEntity;
  friend void ClientObject::onRightClick(Client &client);
  friend void ClientObject::startDeconstructing(void *object);

  void handleInput(double delta);
  void handleLoginInput(double delta);
  void onMouseMove();

  void drawLoadingScreen(const std::string &msg, double progress) const;

  void draw() const;
  mutable bool _drawingFinished;  // Set to true after every redraw.
  void drawTile(size_t x, size_t y, px_t xLoc, px_t yLoc) const;
  void drawTooltip() const;
  // A tooltip which, if it exists, describes the UI element currently moused
  // over.
  const Texture *_uiTooltip;
  MapPoint _offset;  // An offset for drawing, based on the character's location
                     // on the map.
  ScreenPoint _intOffset;  // An integer version of the offset
  void updateOffset();     // Update the offset, when the character moves.

  void drawGearParticles(const ClientItem::vect_t &gear,
                         const MapPoint &location, double delta);

  int _channelsPlaying;  // The number of sound channels currently playing
                         // sounds; updated on tick

  ms_t _time;
  ms_t _timeElapsed;    // Time between last two ticks
  ms_t _lastPingReply;  // The last time a ping reply was received from the
                        // server
  ms_t _lastPingSent;   // The last time a ping was sent to the server
  ms_t _latency;
  unsigned _fps;

  bool _loggedIn;
  bool _loaded;  // Whether the client has sufficient information to begin

  ms_t _timeSinceLocUpdate;  // Time since a CL_LOCATION was sent
  // Location has changed (local or official), and tooltip may have changed.
  bool _tooltipNeedsRefresh;

  // Game data
  bool _dataLoaded;       // If false when run() is called, load default data.
  void initialiseData();  // Any massaging necessary after everything is loaded.
  std::map<char, ClientTerrain> _terrain;
  std::map<std::string, ClientItem> _items;
  std::set<Recipe> _recipes;
  typedef std::set<const ClientObjectType *, ClientObjectType::ptrCompare>
      objectTypes_t;
  objectTypes_t _objectTypes;
  typedef std::set<const ParticleProfile *, ParticleProfile::ptrCompare>
      particleProfiles_t;
  particleProfiles_t _particleProfiles;
  typedef std::set<const Projectile::Type *, Projectile::Type::ptrCompare>
      projectileTypes_t;
  projectileTypes_t _projectileTypes;
  TagNames _tagNames;
  ClientSpells _spells;
  ClientBuffTypes _buffTypes;
  ClassInfo::Container _classes;
  CQuests _quests;

  typedef std::set<SoundProfile> soundProfiles_t;
  soundProfiles_t _soundProfiles;
  const SoundProfile *_avatarSounds{nullptr};
  const SoundProfile *_generalSounds{nullptr};

  Images _icons{"Images/Icons"};

  // Information about the state of the world
  size_t _mapX, _mapY;
  std::vector<std::vector<char> > _map;
  ClientItem::vect_t _inventory;
  std::map<std::string, Avatar *> _otherUsers;  // For lookup by name
  std::map<size_t, ClientObject *> _objects;    // For lookup by serial

  Sprite::set_t _entities;
  void addEntity(Sprite *entity) { _entities.insert(entity); }
  void removeEntity(
      Sprite *const toRemove);  // Remove from _entities, and delete pointer

  Sprite *_currentMouseOverEntity;
  size_t _numEntities;  // Updated every tick
  void addUser(const std::string &name, const MapPoint &location);

  std::unordered_map<ClientTalent::Name, int> _talentLevels;
  std::unordered_map<Tree::Name, int> _pointsInTrees;
  int totalTalentPointsAllocated();

  void applyCollisionChecksToPlayerMovement(MapPoint &pendingDestination) const;
  bool isLocationValidForPlayer(const MapPoint &location) const;
  bool isLocationValidForPlayer(const MapRect &rect) const;

  // Your wars, and your city's wars
  YourWars _warsAgainstPlayers;
  YourWars _warsAgainstCities;
  YourWars _cityWarsAgainstPlayers;
  YourWars _cityWarsAgainstCities;

  std::map<std::string, std::string>
      _userCities;  // Username -> city name. // TODO: remove once users always
                    // exist

  XP _xp = 40;
  XP _maxXP = 100;

  static void onSpellHit(const MapPoint &location, const void *spell);

  mutable LogSDL _debug;

  // Message stuff
 public:
  static std::string compileMessage(MessageCode msgCode,
                                    const std::string &args = "");
  void sendMessage(MessageCode msgCode, const std::string &args = "") const;

 private:
  std::queue<std::string> _messages;
  std::string _partialMessage;
  void sendRawMessage(const std::string &msg = "") const;
  static void sendRawMessageStatic(void *data);
  void handleMessage(const std::string &msg);
  void performCommand(const std::string &commandString);
  std::vector<MessageCode> _messagesReceived;
  std::mutex _messagesReceivedMutex;

 public:
  template <MessageCode code>
  static void sendMessageWithString(void *string) {
    auto pArgs = reinterpret_cast<const std::string *>(string);
    auto message = compileMessage(code, *pArgs);
    sendRawMessageStatic(&message);
  }

 private:
  void handle_SV_LOOTABLE(size_t serial);
  void handle_SV_INVENTORY(size_t serial, size_t slot,
                           const std::string &itemID, size_t quantity);
  void handle_SV_MAX_HEALTH(const std::string &username,
                            Hitpoints newMaxHealth);
  void handle_SV_MAX_ENERGY(const std::string &username, Energy newMaxEnergy);
  void handle_SV_IN_CITY(const std::string &username,
                         const std::string &cityName);
  void handle_SV_NO_CITY(const std::string &cityName);
  void handle_SV_KING(const std::string username);
  void handle_SV_SPELL_HIT(const std::string &spellID, const MapPoint &src,
                           const MapPoint &dst);
  void handle_SV_SPELL_MISS(const std::string &spellID, const MapPoint &src,
                            const MapPoint &dst);
  void handle_SV_RANGED_NPC_HIT(const std::string &npcID, const MapPoint &src,
                                const MapPoint &dst);
  void handle_SV_RANGED_NPC_MISS(const std::string &npcID, const MapPoint &src,
                                 const MapPoint &dst);
  void handle_SV_RANGED_WEAPON_HIT(const std::string &weaponID,
                                   const MapPoint &src, const MapPoint &dst);
  void handle_SV_RANGED_WEAPON_MISS(const std::string &weaponID,
                                    const MapPoint &src, const MapPoint &dst);
  void handle_SV_PLAYER_WAS_HIT(const std::string &username);
  void handle_SV_ENTITY_WAS_HIT(size_t serial);
  void handle_SV_SHOW_OUTCOME_AT(int msgCode, const MapPoint &loc);
  void handle_SV_ENTITY_GOT_BUFF(int msgCode, size_t serial,
                                 const std::string &buffID);
  void handle_SV_PLAYER_GOT_BUFF(int msgCode, const std::string &username,
                                 const std::string &buffID);
  void handle_SV_ENTITY_LOST_BUFF(int msgCode, size_t serial,
                                  const std::string &buffID);
  void handle_SV_PLAYER_LOST_BUFF(int msgCode, const std::string &username,
                                  const std::string &buffID);
  void handle_SV_REMAINING_BUFF_TIME(const std::string &buffID,
                                     ms_t timeRemaining, bool isBuff);
  void handle_SV_KNOWN_SPELLS(const std::set<std::string> &&knownSpellIDs);
  void handle_SV_LEARNED_SPELL(const std::string &spellID);
  void handle_SV_UNLEARNED_SPELL(const std::string &spellID);
  void handle_SV_LEVEL_UP(const std::string &username);
  void handle_SV_NPC_LEVEL(size_t serial, Level level);
  void handle_SV_PLAYER_DAMAGED(const std::string &username, Hitpoints amount);
  void handle_SV_PLAYER_HEALED(const std::string &username, Hitpoints amount);
  void handle_SV_OBJECT_DAMAGED(size_t serial, Hitpoints amount);
  void handle_SV_OBJECT_HEALED(size_t serial, Hitpoints amount);
  void handle_SV_QUEST_CAN_BE_STARTED(const std::string &questID);
  void handle_SV_QUEST_CAN_BE_FINISHED(const std::string &questID);
  void handle_SV_QUEST_COMPLETED(const std::string &questID);
  void handle_SV_QUEST_IN_PROGRESS(const std::string &questID);
  void handle_SV_QUEST_ACCEPTED();
  void handle_SV_QUEST_PROGRESS(const std::string &questID,
                                size_t objectiveIndex, int progress);

  void sendClearTargetMessage() const;

  ConfirmationWindow *_confirmDropItem;
  // Show a confirmation window, then drop item if confirmed
  void dropItemOnConfirmation(size_t serial, size_t slot,
                              const ClientItem *item);

  // Searches
  const ParticleProfile *findParticleProfile(const std::string &id);
  const SoundProfile *findSoundProfile(const std::string &id) const;
  const Projectile::Type *findProjectileType(const std::string &id) const;
  ClientObjectType *findObjectType(const std::string &id);
  ClientNPCType *findNPCType(const std::string &id);

  friend class ContainerGrid;  // Needs to send CL_SWAP_ITEMS messages, and open
                               // a confirmation window
  friend class ClientCombatant;
  friend class ClientObject;
  friend class ClientItem;
  friend class ClientNPC;
  friend class ClientObjectType;
  friend class ClientNPCType;
  friend class CDataLoader;
  friend class Connection;
  friend class Projectile;
  friend class ClientVehicle;
  friend void LogSDL::operator()(const std::string &message,
                                 const Color &color);
  friend class ItemSelector;
  friend void Window::hideWindow(void *window);
  friend class TakeContainer;
  friend class TestClient;
  friend class Avatar;
  friend class SoundProfile;
  friend class Sprite;
  friend class Target;
  friend class ConfirmationWindow;
  friend class InfoWindow;
  friend class InputWindow;
  friend class Window;
};

#endif
