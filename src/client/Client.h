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
#include "../Map.h"
#include "../Point.h"
#include "../Rect.h"
#include "../Serial.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../types.h"
#include "Avatar.h"
#include "CCities.h"
#include "CDroppedItem.h"
#include "CGameData.h"
#include "CQuest.h"
#include "CRecipe.h"
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
#include "CurrentTools.h"
#include "HelpEntry.h"
#include "KeyboardStateFetcher.h"
#include "LogSDL.h"
#include "MemoisedImageDirectory.h"
#include "ParticleProfile.h"
#include "Projectile.h"
#include "SoundProfile.h"
#include "Sprite.h"
#include "Tag.h"
#include "Target.h"
#include "WordWrapper.h"
#include "drawing.h"
#include "ui/ChoiceList.h"
#include "ui/ContainerGrid.h"
#include "ui/Indicator.h"
#include "ui/ItemSelector.h"
#include "ui/OutlinedLabel.h"
#include "ui/Window.h"

struct GroupUI;
class TextBox;

class Client : public TextEntryManager {
 public:
  Client();
  ~Client();
  static void initStatics();
  static void cleanUpStatics();

  void run();
  void gameLoop();
  void loginScreenLoop();  // Alternative game loop to run()

  TTF_Font *defaultFont() const;

  void showErrorMessage(const std::string &message, Color color) const;
  std::vector<std::string> _queuedErrorMessagesFromOtherThreads;
  void showQueuedErrorMessages();

  struct Toast {
    ms_t timeRemaining;
    Texture icon;
    std::string text;
  };
  std::list<Toast> toastInfo;
  std::vector<std::string> _queuedToastsFromOtherThreads;

  const Avatar &character() const { return _character; }
  const ScreenPoint &offset() const { return _intOffset; }
  const std::string &username() const { return _username; }
  const Sprite *currentMouseOverEntity() const {
    return _currentMouseOverEntity;
  }
  MapRect playerCollisionRect() const { return _character.collisionRect(); }
  ClientCombatant *targetAsCombatant() const { return _target.combatant(); }
  const Sprite *targetAsEntity() const { return _target.entity(); }
  const SoundProfile *avatarSounds() const { return _avatarSounds; }
  const SoundProfile *generalSounds() const { return _generalSounds; }

  const HelpEntries &helpEntries() const { return _helpWindow.entries; }
  const Window &helpWindow() const { return *_helpWindow.window; }
  XP xp() const { return _xp; }

  bool isAtWarWith(const Avatar &user) const;
  bool isAtWarWithObjectOwner(const ClientObject::Owner &owner) const;
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
  std::string getUserCity(const std::string &name) const;

  ConfirmationWindow *_groupInvitationWindow{nullptr};

  // Special types
  SpriteType avatarSpriteType;
  ClientCombatantType avatarCombatantType;
  CDroppedItem::Type droppedItemType;

  const ClientSpell *findSpell(const std::string &spellID) const;

  template <typename T>
  void setTarget(T &newTarget, bool aggressive = false) {
    _target.setAndAlertServer(newTarget, aggressive);
  }
  void clearTarget() {
    _target.clear(*this);
    refreshTargetBuffs();
  }
  void hideTargetMenu() { _target.hideMenu(); }

  static const px_t ICON_SIZE;
  static const double MOVEMENT_SPEED;
  static const Hitpoints MAX_PLAYER_HEALTH;

  static const size_t INVENTORY_SIZE;
  static const size_t GEAR_SLOTS;

  static const px_t ACTION_DISTANCE;

  typedef std::list<Window *> windows_t;
  typedef std::list<Element *>
      ui_t;  // For the UI, that sits below all windows.

  const ScreenPoint &mouse() const { return _mouse; }

  void addWindow(Window *window);
  void removeWindow(Window *window);  // Linear time
  void showWindowInFront(Window *window);
  static void showWindowInFront(void *window);
  bool isWindowRegistered(const Window *toFind);

  static const px_t SCREEN_X, SCREEN_Y;

  static const px_t CULL_DISTANCE, CULL_HYSTERESIS_DISTANCE;

  static const int MIXING_CHANNELS;

  static std::vector<std::string> GEAR_SLOT_NAMES;

  static const size_t NUM_HOTBAR_BUTTONS{13};

  void infoWindow(const std::string &text);

  void showHelpTopic(const std::string &topic) {
    _helpWindow.showHelpTopic(topic);
  }

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
                    double delta);  // per second
  void addParticles(const std::string &profileName,
                    const MapPoint &location);  // Single hit
  void addParticles(const std::string &profileName, const MapPoint &location,
                    double delta);  // per second
  void addParticlesWithCustomAltitude(double altitude,
                                      const std::string &profileName,
                                      const MapPoint &location,
                                      double delta);  // per second
  void addFloatingCombatText(const std::string &text, const MapPoint &location,
                             Color color);

  ContainerGrid::GridInUse containerGridInUse, containerGridBeingDraggedFrom;

 private:
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
  ms_t TIME_BETWEEN_LOCATION_UPDATES = 250;

  const Texture *_currentCursor;

  static void initializeGearSlotNames();

  // Whether the user has the specified item(s).
  bool playerHasItem(const Item *item, size_t quantity = 1) const;

  static void initializeCraftingWindow(Client &client);
  void initializeCraftingWindow();
  bool _haveMatsFilter, _haveToolsFilter, _tagOr, _matOr;
  // The height of windows' section headings
  static const px_t HEADING_HEIGHT;
  // The total height occupied by a line and its surrounding spacing
  static const px_t LINE_GAP;
  // The recipe currently selected, if any
  const CRecipe *_activeRecipe{nullptr};
  // Called when the "Craft" button is clicked.  0 = infinite.
  void startCrafting(int quantity);
  // Populated at load time, after _items
  std::map<std::string, bool> _tagFilters;
  std::map<const ClientItem *, bool> _matFilters;
  mutable bool _tagFilterSelected,
      _matFilterSelected;  // Whether any filters have been selected
  bool recipeMatchesFilters(const CRecipe &recipe) const;
  // Called when filters pane is clicked, or new recipes are unlocked.
  static void populateRecipesList(Element &e);
  // Called when filters change
  void scrollRecipeListToTop();
  // Called when a recipe is selected.
  static void onClickRecipe(Element &e, const ScreenPoint &mousePos);
  void refreshRecipeDetailsPane();
  // Called when new recipes are unlocked.
  void populateFilters();
  // Called when new construction items are unlocked
  void populateBuildList();

  bool outsideCullRange(const MapPoint &loc, px_t hysteresis = 0) const;
  void closeWindowsFromOutOfRangeObjects();

  ChoiceList *_recipeList = nullptr;
  Element *_detailsPane = nullptr;
  List *_tagList = nullptr, *_materialsList = nullptr;

  Window *_craftingWindow = nullptr, *_buildWindow = nullptr;
  std::set<std::string> _knownRecipes, _knownConstructions;

  void initializeBuildWindow();
  ChoiceList *_buildList = nullptr;
  static void chooseConstruction(Element &e, const ScreenPoint &mousePos);
  const ClientObjectType *_selectedConstruction{nullptr};
  bool _multiBuild{false};

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
  Picture *_mapPicture{nullptr};
  Element *_mapPins{nullptr}, *_mapPinOutlines{nullptr}, *_mapIcons{nullptr};
  static const px_t MAP_IMAGE_W = 300, MAP_IMAGE_H = 300;
  void initializeMapWindow();
  static void updateMapWindow(Element &);
  void Client::addMapPin(const MapPoint &worldPosition, const Color &color,
                         const std::string &tooltip = {});
  void Client::addOutlinedMapPin(const MapPoint &worldPosition,
                                 const Color &color);
  void Client::addIconToMap(const MapPoint &worldPosition, const Texture *icon,
                            const std::string &tooltip = {});
  ScreenRect Client::convertToMapPosition(const MapPoint &worldPosition) const;
  int _zoom{3};
  static const int MIN_ZOOM{0};
  static const int MAX_ZOOM{5};
  void zoomMapIn();
  void zoomMapOut();
  static void onMapScrollUp(Element &);
  static void onMapScrollDown(Element &);
  Button *_zoomMapInButton{nullptr};
  Button *_zoomMapOutButton{nullptr};
  MapPoint _respawnPoint;
  struct MapWindow {
    int zoomMultiplier{0};
    ScreenPoint displacement;
    Picture *fogOfWar{nullptr};
    TextBox *objectFilter{nullptr};
  } mapWindow;

  // Social window
  Window *_socialWindow{nullptr};
  void initializeSocialWindow();
  List *_warsList{nullptr};
  Element *_citySection{nullptr};
  void refreshCitySection();
  void populateWarsList();
  Label *_numOnlinePlayersLabel{nullptr};
  List *_onlinePlayersList{nullptr};
  void populateOnlinePlayersList();
  std::set<std::string> _allOnlinePlayers{};

  struct HelpWindow {
    void initialise(Client &client);
    void loadEntries();
    void showHelpTopic(const std::string &topic);
    Window *window{nullptr};
    HelpEntries entries;
  } _helpWindow;

  Window *_classWindow{nullptr};
  Element *_talentTrees{nullptr};
  Label *_levelLabel{nullptr};
  Label *_xpLabel{nullptr};
  Label *_pointsAllocatedLabel{nullptr};
  void initializeClassWindow();
  void confirmAndUnlearnTalents();
  void populateClassWindow();
  std::set<const ClientSpell *> _knownSpells{};
  std::map<std::string, ms_t> _spellCooldowns{};

  Window *_bugReportWindow{nullptr};

  windows_t _windows;

  ui_t _ui;
  void addUI(Element *element);
  Element *_castBar;
  Element *_hotbar{nullptr};
  std::vector<Button *> _hotbarButtons = {NUM_HOTBAR_BUTTONS, nullptr};
  std::vector<OutlinedLabel *> _hotbarCooldownLabels = {NUM_HOTBAR_BUTTONS,
                                                        nullptr};
  std::map<ClientBuffType::ID, ms_t> _buffTimeRemaining{},
      _debuffTimeRemaining{};  // Used for the UI only.
  List *_buffsDisplay{nullptr};
  Element *_targetBuffs{nullptr};
  OutlinedLabel *_lastErrorMessage{nullptr};
  mutable ms_t _errorMessageTimer{0};
  Window *_escapeWindow{nullptr};
  Window *_optionsWindow{nullptr};
  List *_questProgress{nullptr};
  Element *_toolsDisplay{nullptr};
  void refreshTools() const;

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
  void setHotbar(const std::vector<std::pair<int, std::string> > &buttons);

  ConfirmationWindow *_unlearnTalentsConfirmationWindow{nullptr};

 public:
  struct Hotbar {
    Window *assigner{nullptr};
    List *spellList{nullptr}, *recipeList{nullptr};
    ChoiceList *categoryList{nullptr};
  } hotbar;
  void refreshHotbar();
  void refreshQuestProgress();
  void refreshHotbarCooldowns();

  GroupUI *groupUI{nullptr};

 private:
  void initAssignerWindow();
  void populateAssignerWindow();
  void onHotbarKeyDown(SDL_Keycode key);
  void onHotbarKeyUp(SDL_Keycode key);
  void initBuffsDisplay();
  void refreshBuffsDisplay();
  static Element *assembleBuffEntry(Client &client, const ClientBuffType &type,
                                    bool isDebuff = false);
  void initTargetBuffs();
  void refreshTargetBuffs();
  void initializeEscapeWindow();
  void initialiseOptionsWindow();
  void updateUI();
  List *_toasts{nullptr};
  void initToasts();
  void updateToasts();
  void populateToastsList();
  void toast(const std::string &icon, const std::string &message);
  void toast(const Texture &icon, const std::string &message);
  void initQuestProgress();
  Button *_skipTutorialButton{nullptr};
  ConfirmationWindow *_skipTutorialConfirmation{nullptr};
  void initSkipTutorialButton();
  OutlinedLabel *_instructionsLabel{nullptr};

  // Chat
  Element *_chatContainer;
  List *_chatLog;
  TextBox *_chatTextBox;
  void addChatMessage(const std::string &msg,
                      const Color &color = Color::UI_FEEDBACK);
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
  std::string _allowedTerrain{};  // TerrainList ID.  Empty: default.

  // Login screen
  ui_t _loginUI;
  void initLoginScreen();
  void drawLoginScreen() const;
  std::list<Particle *> _loginParticles;
  void updateLoginParticles(double delta);
  void login();
  Indicator *_serverConnectionIndicator{nullptr};
  void updateLoginButton();
  Window *_createWindow{nullptr};
  void initCreateWindow();
  void createAccount();
  static void updateCreateButton(void *pClient);
  static void updateClassDescription(Client &client);
  std::string _autoClassID =
      {};  // For automated account creation, i.e., in tests
  std::string _savedPwHash;
  enum ReleaseNotesStatus {
    RELEASE_NOTES_DOWNLOADING,
    RELEASE_NOTES_DOWNLOADED,
    RELEASE_NOTES_DISPLAYED
  } _releaseNotesStatus{RELEASE_NOTES_DOWNLOADING};
  std::string _releaseNotesRaw;
  void showReleaseNotes();
  struct LoginScreenElements {
    TextBox *nameBox{nullptr};
    TextBox *newNameBox{nullptr};
    TextBox *pwBox{nullptr};
    TextBox *newPwBox{nullptr};
    Button *loginButton{nullptr};
    Button *createButton{nullptr};
    ChoiceList *classList{nullptr};
    List *classDescription{nullptr};
    OutlinedLabel *loginErrorLabel{nullptr};
    Window *donateWindow{nullptr};
    List *releaseNotes{nullptr};
  } loginScreenElements;

  // These are superficial, and relate only to the cast bar.
  ms_t _actionTimer{0};        // How long the character has been performing the
                               // current action.
  ms_t _actionLength{0};       // How long the current action should take.
  std::string _actionMessage;  // A description of the current action.
 public:
  void prepareAction(const std::string &msg);  // Set up the action, awaiting
                                               // server confirmation.
 private:
  void startAction(
      ms_t actionLength);  // Start the action timer.  If zero, stop the timer.

  Target _target;

  bool _loop{true};

  enum State { STARTING_UP, RUNNING, FINISHED };
  // True while run() is being executed.
  State _state{STARTING_UP};
  // For testing purposes only; should otherwise remain false.
  bool _freeze{false};
  bool _shouldAutoLogIn{false};  // Used for tests
  static void exitGame(void *client);

  static TTF_Font *_defaultFont;

  // Mouse stuff
  ScreenPoint _mouse{0, 0};  // Mouse position
  bool _mouseMoved{false};
  Sprite *getEntityAtMouse();
  void
  checkMouseOver();  // Set state based on window/entity/etc. being moused over.
  bool _mouseOverWindow{
      false};  // Whether the mouse is over any visible window.

  bool _leftMouseDown{false};  // Whether the left mouse button is pressed
  friend void ClientObject::onLeftClick();

  bool _rightMouseDown{false};
  friend void ClientObject::onRightClick();
  friend void ClientObject::startDeconstructing(void *object);

  std::set<ContainerGrid *> _containerGrids;

 public:
  void registerContainerGrid(ContainerGrid *cg) { _containerGrids.insert(cg); }
  void deregisterContainerGrid(ContainerGrid *cg) { _containerGrids.erase(cg); }

  static bool isCtrlPressed();
  static bool isAltPressed();
  static bool isShiftPressed();

 private:
  void handleInput(double delta);
  void handleLoginInput(double delta);
  void onMouseMove();

  void drawLoadingScreen(const std::string &msg) const;
  mutable int _loadingScreenProgress{0};

  void draw() const;
  mutable bool _drawingFinished{false};  // Set to true after every redraw.
  void drawTile(size_t x, size_t y, px_t xLoc, px_t yLoc) const;
  void drawTooltip() const;
  // A tooltip which, if it exists, describes the UI element currently moused
  // over.
  const Texture *_uiTooltip;
  mutable const ClientObjectType *_constructionFootprintType{nullptr};
  mutable const TerrainList *_constructionFootprintAllowedTerrain{nullptr};

 public:
  void drawFootprint(const MapRect &rect, Color color,
                     Uint8 alpha = 0xff) const;
  void drawSelectionCircle() const;

 private:
  MapPoint _offset;  // An offset for drawing, based on the character's location
                     // on the map.
  ScreenPoint _intOffset;  // An integer version of the offset
  void updateOffset();     // Update the offset, when the character moves.

  void drawGearParticles(const ClientItem::vect_t &gear,
                         const MapPoint &location, double delta);

  int _channelsPlaying{0};  // The number of sound channels currently playing
                            // sounds; updated on tick

  ms_t _time;
  ms_t _timeElapsed{0};  // Time between last two ticks
  ms_t _lastPingReply;   // The last time a ping reply was received from the
                         // server
  ms_t _lastPingSent;    // The last time a ping was sent to the server
  ms_t _latency{0};
  unsigned _fps{0};

  bool _loggedIn{false};
  // Whether the client has sufficient information to begin
  bool _loaded{false};

  MapPoint _lastDirection{};
  ms_t _timeSinceLocUpdate{0};  // Time since a CL_MOVE_TO was sent
  // Location has changed (local or official), and tooltip may have changed.
  bool _tooltipNeedsRefresh{false};

 public:
  // Game data
  CGameData gameData;

  static struct CommonImages {
    void initialise();

    Texture shadow;
    Texture cursorNormal, cursorGather, cursorContainer, cursorAttack,
        cursorStartsQuest, cursorEndsQuest, cursorRepair, cursorVehicle,
        cursorText;
    Texture startQuestIcon, endQuestIcon;
    Texture startQuestIndicator, endQuestIndicator;
    Texture eliteWreath, bossWreath;

    Texture itemHighlightMouseOver, itemHighlightMatch, itemHighlightNoMatch;

    Texture itemQualityMask;
    Texture cityIcon, playerIcon;

    Texture loginBackgroundBack, loginBackgroundFront;

    Texture basePassive, baseAggressive;

    Texture scrollArrowWhiteUp, scrollArrowWhiteDown, scrollArrowGreyUp,
        scrollArrowGreyDown;

    Texture constructionPeg;

    MemoisedImageDirectory icons;
    MemoisedImageDirectory toolIcons{Color::MAGENTA};

    Texture map;
    Texture mapCityFriendly, mapCityNeutral, mapCityEnemy;
    Texture mapRespawn;

    Texture logoDiscord, logoBTC, btcQR;
  } images;

 private:
  bool _dataLoaded;       // If false when run() is called, load default data.
  void initialiseData();  // Any massaging necessary after everything is loaded.

  const SoundProfile *_avatarSounds{nullptr};
  const SoundProfile *_generalSounds{nullptr};

  // Information about the state of the world
  Map _map;
  ClientItem::vect_t _inventory;
  std::map<std::string, Avatar *> _otherUsers;  // For lookup by name
  std::map<Serial, ClientObject *> _objects;    // For lookup by serial
  char _terrainUnderCursor{-1};
  Tooltip _terrainTooltip;
  void updateTerrainTooltip();

  static const size_t TILES_PER_CHUNK = 10;
  std::vector<std::vector<bool> > _mapExplored;
  Texture _fogOfWar;
  void clearChunkFromFogOfWar(size_t x, size_t y);
  void redrawFogOfWar();

  Sprite::set_t _entities;
  void addEntity(Sprite *entity);
  void removeEntity(
      Sprite *const toRemove);  // Remove from _entities, and delete pointer
  void cullObjects();

  Sprite *_currentMouseOverEntity{nullptr};
  size_t _numEntities{0};  // Updated every tick
  void addUser(const std::string &name, const MapPoint &location);

  std::unordered_map<ClientTalent::Name, int> _talentLevels;
  std::unordered_map<Tree::Name, int> _pointsInTrees;
  int totalTalentPointsAllocated();

  CurrentTools _currentTools;

  void applyCollisionChecksToPlayerMovement(MapPoint &pendingDestination) const;
  bool isLocationValidForPlayer(const MapPoint &location) const;
  bool isLocationValidForPlayer(const MapRect &rect) const;

  CCities _cities;

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

  // Note: for now, this is used only for permanentObjects.
  Sprite::set_t _spritesWithCustomCullDistances;

  KeyboardStateFetcher _keyboardState;

  static void onSpellHit(const MapPoint &location, void *spell);

  mutable LogSDL _debug;

  // Message stuff
 public:
  void sendMessage(const Message &msg) const;

 private:
  std::queue<std::string> _messages;
  std::string _partialMessage;
  void handleBufferedMessages(const std::string &msg);
  void performCommand(const std::string &commandString);
  std::vector<MessageCode> _messagesReceived;
  std::mutex _messagesReceivedMutex;

 private:
  void handle_SV_INVENTORY(Serial serial, size_t slot,
                           const std::string &itemID, size_t quantity,
                           Hitpoints itemHealth, bool isSoulbound,
                           std::string suffixID);
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
  void handle_SV_ENTITY_WAS_HIT(Serial serial);
  void handle_SV_SHOW_OUTCOME_AT(int msgCode, const MapPoint &loc);
  void handle_SV_ENTITY_GOT_BUFF(int msgCode, Serial serial,
                                 const std::string &buffID);
  void handle_SV_PLAYER_GOT_BUFF(int msgCode, const std::string &username,
                                 const std::string &buffID);
  void handle_SV_ENTITY_LOST_BUFF(int msgCode, Serial serial,
                                  const std::string &buffID);
  void handle_SV_PLAYER_LOST_BUFF(int msgCode, const std::string &username,
                                  const std::string &buffID);
  void handle_SV_REMAINING_BUFF_TIME(const std::string &buffID,
                                     ms_t timeRemaining, bool isBuff);
  void handle_SV_KNOWN_SPELLS(const std::set<std::string> &&knownSpellIDs);
  void handle_SV_LEARNED_SPELL(const std::string &spellID);
  void handle_SV_UNLEARNED_SPELL(const std::string &spellID);
  void handle_SV_LEVEL_UP(const std::string &username);
  void handle_SV_NPC_LEVEL(Serial serial, Level level);
  void handle_SV_PLAYER_DAMAGED(const std::string &username, Hitpoints amount);
  void handle_SV_PLAYER_HEALED(const std::string &username, Hitpoints amount);
  void handle_SV_OBJECT_DAMAGED(Serial serial, Hitpoints amount);
  void handle_SV_OBJECT_HEALED(Serial serial, Hitpoints amount);
  void handle_SV_QUEST_CAN_BE_STARTED(const std::string &questID);
  void handle_SV_QUEST_CAN_BE_FINISHED(const std::string &questID);
  void handle_SV_QUEST_COMPLETED(const std::string &questID);
  void handle_SV_QUEST_IN_PROGRESS(const std::string &questID);
  void handle_SV_QUEST_ACCEPTED();
  void handle_SV_QUEST_PROGRESS(const std::string &questID,
                                size_t objectiveIndex, int progress);
  void handle_SV_MAP_EXPLORATION_DATA(size_t column, std::vector<Uint32> data);
  void handle_SV_CHUNK_EXPLORED(size_t chunkX, size_t chunkY);

  void sendClearTargetMessage() const;

  ConfirmationWindow *_confirmDropSoulboundItem = nullptr;
  // Show a confirmation window, then drop item if confirmed
  void dropItemOnConfirmation(const ContainerGrid::GridInUse &toDrop);
  ConfirmationWindow *_confirmLootSoulboundItem = nullptr;
  void scrapItemOnConfirmation(const ContainerGrid::GridInUse &toDrop,
                               std::string itemName);
  ConfirmationWindow *_confirmScrapItem = nullptr;

  // Searches
 public:
  const ParticleProfile *findParticleProfile(const std::string &id) const;
  const SoundProfile *findSoundProfile(const std::string &id) const;
  const Projectile::Type *findProjectileType(const std::string &id) const;
  ClientObjectType *findObjectType(const std::string &id);
  ClientNPCType *findNPCType(const std::string &id);
  const ClientItem *findItem(const std::string &id) const;
  const CNPCTemplate *findNPCTemplate(const std::string &id) const;
  Avatar *findUser(const std::string &username);

 private:
  friend class ContainerGrid;  // Needs to send CL_SWAP_ITEMS messages, and open
                               // a confirmation window
  friend class ClientCombatant;
  friend class ClientObject;
  friend class ClientItem;
  friend class ClientNPC;
  friend class ClientObjectType;
  friend class ClientNPCType;
  friend class CurrentTools;
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
