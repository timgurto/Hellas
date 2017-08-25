#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "Avatar.h"
#include "ClientCombatant.h"
#include "ClientConfig.h"
#include "ClientItem.h"
#include "ClientMerchantSlot.h"
#include "ClientNPC.h"
#include "ClientObject.h"
#include "ClientObjectType.h"
#include "ClientTerrain.h"
#include "Sprite.h"
#include "LogSDL.h"
#include "ParticleProfile.h"
#include "SoundProfile.h"
#include "Target.h"
#include "ui/ChoiceList.h"
#include "ui/ItemSelector.h"
#include "ui/Window.h"
#include "../Args.h"
#include "../Point.h"
#include "../Rect.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../types.h"
#include "../server/Recipe.h"

class TextBox;

class Client{
public:
    Client();
    ~Client();
    void run();
    void gameLoop();
    void loginScreenLoop(); // Alternative game loop to run()

    static bool isClient;

    const Socket &socket() const;
    TTF_Font *defaultFont() const;

    const Avatar &character() const { return _character; }
    const Point &offset() const { return _intOffset; }
    const std::string &username() const { return _username; }
    const Sprite *currentMouseOverEntity() const { return _currentMouseOverEntity; }
    Rect playerCollisionRect() const { return _character.collisionRect(); }
    const ClientCombatant *targetAsCombatant() const { return _target.combatant(); }
    const Sprite *targetAsEntity() const { return _target.entity(); }
    bool isDismounting() const { return _isDismounting; }
    void attemptDismount() { _isDismounting = true; }
    const SoundProfile *avatarSounds() const { return _avatarSounds; }

    bool isAtWarWith(const Avatar &user) const;
    bool isAtWarWith(const std::string &username) const;
    bool isAtWarWithPlayerDirectly(const std::string &username) const{
        return _atWarWithPlayer.find(username) != _atWarWithPlayer.end(); }
    bool isAtWarWithCityDirectly(const std::string &cityName) const{
        return _atWarWithCity.find(cityName) != _atWarWithCity.end(); }

    const Texture &mapImage() const { return _mapImage; }

    template<typename T>
    void setTarget(const T &newTarget, bool aggressive = false){
        _target.setAndAlertServer(newTarget, aggressive);
    }
    void clearTarget() { _target.clear(); }
    void hideTargetMenu(){ _target.hideMenu(); }
    
    const Texture &cursorNormal() const { return _cursorNormal; }
    const Texture &cursorGather() const { return _cursorGather; }
    const Texture &cursorContainer() const { return _cursorContainer; }
    const Texture &cursorAttack() const { return _cursorAttack; }

    static const px_t ICON_SIZE;
    static const px_t TILE_W, TILE_H;
    static const double MOVEMENT_SPEED;
    static const double VEHICLE_SPEED;
    static const health_t MAX_PLAYER_HEALTH;

    enum SpecialSerial{
        INVENTORY = 0,
        GEAR = 1,
    };
    static const size_t INVENTORY_SIZE;
    static const size_t GEAR_SLOTS;

    static const px_t ACTION_DISTANCE;

    static LogSDL &debug() { return *_debugInstance; }

    typedef std::list<Window *> windows_t;
    typedef std::list<Element *> ui_t; // For the UI, that sits below all windows.

    static const px_t
        SCREEN_X,
        SCREEN_Y;

    static const px_t
        CULL_DISTANCE,
        CULL_HYSTERESIS_DISTANCE;

    static const int MIXING_CHANNELS;
    
    static std::vector<std::string> GEAR_SLOT_NAMES;

private:
    static Client *_instance;
    static LogSDL *_debugInstance;

    static std::map<std::string, int> _messageCommands;
    static std::map<int, std::string> _errorMessages;
    static void initializeMessageNames();

    ClientConfig _config;

    std::string _defaultServerAddress;

    static const ms_t MAX_TICK_LENGTH;
    static const ms_t SERVER_TIMEOUT; // How long the client will wait for a ping reply
    static const ms_t CONNECT_RETRY_DELAY; // How long to wait between retries at connecting
    static const ms_t PING_FREQUENCY; // How often to test latency with each client
    // How often to send location updates to server (while moving)
    static const ms_t TIME_BETWEEN_LOCATION_UPDATES;

    Texture
        _cursorNormal,
        _cursorGather,
        _cursorContainer,
        _cursorAttack;
    const Texture *_currentCursor;

    bool _isDismounting; // Whether the user is looking for a dismount location.

    static void initializeGearSlotNames();

    // Whether the user has the specified item(s).
    bool playerHasItem(const Item *item, size_t quantity = 1) const;
    
    static void initializeCraftingWindow(Client &client);
    void initializeCraftingWindow();
    bool _haveMatsFilter, _haveToolsFilter, _tagOr, _matOr;
    static const px_t HEADING_HEIGHT; // The height of windows' section headings
    static const px_t LINE_GAP; // The total height occupied by a line and its surrounding spacing
    const Recipe *_activeRecipe; // The recipe currently selected, if any
    static void startCrafting(void *data); // Called when the "Craft" button is clicked.
    // Populated at load time, after _items
    std::map<std::string, bool> _tagFilters;
    std::map<const ClientItem *, bool> _matFilters;
    mutable bool _tagFilterSelected, _matFilterSelected; // Whether any filters have been selected
    bool recipeMatchesFilters(const Recipe &recipe) const;
    // Called when filters pane is clicked, or new recipes are unlocked.
    static void populateRecipesList(Element &e);
    // Called when a recipe is selected.
    static void selectRecipe(Element &e, const Point &mousePos);
    // Called when new recipes are unlocked.
    void populateFilters();
    // Called when new construction items are unlocked
    void populateBuildList();

    std::set<ClientObject *> _objectsWatched;
    void watchObject(ClientObject &obj);
    void unwatchObject(ClientObject &obj);

    bool outsideCullRange(const Point &loc, px_t hysteresis = 0) const;

    ChoiceList *_recipeList;
    Element *_detailsPane;
    List
        *_tagList,
        *_materialsList;

    Window 
        *_craftingWindow,
        *_buildWindow;
    std::set<std::string>
        _knownRecipes,
        _knownConstructions;

    void initializeBuildWindow();
    ChoiceList *_buildList;
    static void chooseConstruction(Element &e, const Point &mousePos);
    const ClientObjectType *_selectedConstruction;
    bool _multiBuild;

    Window *_inventoryWindow;
    void initializeInventoryWindow();
    Texture _constructionFootprint;

    Window *_gearWindow;
    static void initializeGearWindow(Client &client);
    void initializeGearWindow();
    Texture _gearWindowBackground;
    void onChangeDragItem(){ _gearWindow->forceRefresh(); }

    Window *_mapWindow;
    Texture _mapImage;
    Element *_mapPins, *_mapPinOutlines;
    void initializeMapWindow();
    static void updateMapWindow(Element &);
    void addMapPin(const Point &position, const Color &color);

    windows_t _windows;
    void addWindow(Window *window);
    void removeWindow(Window *window); // Linear time

    ui_t _ui;
    void addUI(Element *element);
    Element *_castBar;
    void initUI();
        void initChatLog();
        void initWindows();
        void initCastBar();
        void initMenuBar();
        void addButtonToMenu(Element *menuBar, size_t index, Element *toToggle,
                const std::string iconFile, const std::string tooltip = "");
        void initPerformanceDisplay();
        void initPlayerPanels();

    // Chat
    Element *_chatContainer;
    List *_chatLog;
    TextBox *_chatTextBox;
    void addChatMessage(const std::string &msg, const Color &color = Color::FONT);
    static Color
        SAY_COLOR,
        WHISPER_COLOR;
    std::string _lastWhisperer; // The username of the last person to whisper, for fast replying.

    std::string _username;
    void initializeUsername();
    void setRandomUsername();
    Avatar _character; // Describes the user's character
    Stats _stats; // The user's stats
    Point _pendingCharLoc; // Where the player has told his character to go. Unconfirmed by server.

    // Login screen
    Texture _loginFront, _loginBack;
    ui_t _loginUI;
    void initLoginScreen();
    void cleanUpLoginScreen();
    void drawLoginScreen() const;
    std::list<Particle*> _loginParticles;
    void updateLoginParticles(double delta);
    static void login(void *);

    // These are superficial, and relate only to the cast bar.
    ms_t _actionTimer; // How long the character has been performing the current action.
    ms_t _actionLength; // How long the current action should take.
    std::string _actionMessage; // A description of the current action.
    void prepareAction(const std::string &msg); // Set up the action, awaiting server confirmation.
    void startAction(ms_t actionLength); // Start the action timer.  If zero, stop the timer.

    Target _target;
    /*
    Whether the player is targeting aggressively, i.e., will attack when in range.
    Used here to decide when to alert server of new targets.
    */
    Texture
        _basePassive,
        _baseAggressive;
    bool _rightMouseDownWasOnUI;

    bool _loop;
    bool _running; // True while run() is being executed.
    bool _freeze; // For testing purposes only; should otherwise remain false.
    Socket _socket;

    TTF_Font *_defaultFont;

    // Mouse stuff
    Point _mouse; // Mouse position
    bool _mouseMoved;
    Sprite *getEntityAtMouse();
    void checkMouseOver(); // Set state based on window/entity/etc. being moused over.
    bool _mouseOverWindow; // Whether the mouse is over any visible window.

    bool _leftMouseDown; // Whether the left mouse button is pressed
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
    mutable bool _drawingFinished; // Set to true after every redraw.
    void drawTile(size_t x, size_t y, px_t xLoc, px_t yLoc) const;
    void drawTooltip() const;
    // A tooltip which, if it exists, describes the UI element currently moused over.
    const Texture *_uiTooltip;
    Point _offset; // An offset for drawing, based on the character's location on the map.
    Point _intOffset; // An integer version of the offset
    void updateOffset(); // Update the offset, when the character moves.

    int _channelsPlaying; // The number of sound channels currently playing sounds; updated on tick

    ms_t _time;
    ms_t _timeElapsed; // Time between last two ticks
    ms_t _lastPingReply; // The last time a ping reply was received from the server
    ms_t _lastPingSent; // The last time a ping was sent to the server
    ms_t _latency;
    ms_t _timeSinceConnectAttempt;
    unsigned _fps;

    bool _invalidUsername; // Flag set if server refused username

    bool _loggedIn;
    bool _loaded; // Whether the client has sufficient information to begin

    static const size_t BUFFER_SIZE = 1023;

    ms_t _timeSinceLocUpdate; // Time since a CL_LOCATION was sent
    // Location has changed (local or official), and tooltip may have changed.
    bool _tooltipNeedsRefresh;

    // Game data
    void loadData(const std::string &path = "Data");
    bool _dataLoaded; // If false when run() is called, load default data.
    std::map<char, ClientTerrain> _terrain;
    std::map<std::string, ClientItem> _items;
    std::set<Recipe> _recipes;
    typedef std::set<const ClientObjectType*, ClientObjectType::ptrCompare> objectTypes_t;
    objectTypes_t _objectTypes;
    typedef std::set<const ParticleProfile*, ParticleProfile::ptrCompare> particleProfiles_t;
    particleProfiles_t _particleProfiles;

    typedef std::set<SoundProfile> soundProfiles_t;
    soundProfiles_t _soundProfiles;
    const SoundProfile *_avatarSounds;
    

    // Information about the state of the world
    size_t _mapX, _mapY;
    std::vector<std::vector<char> > _map;
    ClientItem::vect_t _inventory;
    std::map<std::string, Avatar*> _otherUsers; // For lookup by name
    std::map<size_t, ClientObject*> _objects; // For lookup by serial

    Sprite::set_t _entities;
    void addEntity(Sprite *entity) { _entities.insert(entity); }
    void removeEntity(Sprite *const toRemove); // Remove from _entities, and delete pointer
    // Move the entity, and reorder it if necessary
    void setEntityLocation(Sprite *entity, const Point &location);
    Sprite *_currentMouseOverEntity;
    size_t _numEntities; // Updated every tick
    void addUser(const std::string &name, const Point &location);
    
    // Your wars, or if you're in a city, your city's wars
    std::set<std::string> _atWarWithPlayer;
    std::set<std::string> _atWarWithCity;

    std::map<std::string, std::string> _userCities; // Username -> city name
    
    void addParticles(const ParticleProfile *profile, const Point &location, size_t qty);
    void addParticles(const ParticleProfile *profile, const Point &location); // Single hit
    void addParticles(const ParticleProfile *profile, const Point &location, double delta);  // /s
    void addParticles(const std::string &profileName, const Point &location); // Single hit
    void addParticles(const std::string &profileName, const Point &location, double delta);  // /s


    mutable LogSDL _debug;

    // Message stuff
    std::queue<std::string> _messages;
    std::string _partialMessage;
    void sendRawMessage(const std::string &msg = "") const;
    void sendMessage(MessageCode msgCode, const std::string &args = "") const;
    void handleMessage(const std::string &msg);
    void performCommand(const std::string &commandString);
    std::vector<MessageCode> _messagesReceived;

    void handle_SV_LOOTABLE(size_t serial);
    void handle_SV_INVENTORY(size_t serial, size_t slot, const std::string &itemID, size_t quantity);
    void handle_SV_IN_CITY(const std::string &username, const std::string &cityName);

    void sendClearTargetMessage() const;
    
    enum ConnectionStatus{
        INITIALIZING,
        IN_LOGIN_SCREEN,
        TRYING_TO_CONNECT,
        CONNECTED,
        LOGGED_IN,
        LOADED,
        CONNECTION_ERROR
    };
    ConnectionStatus _connectionStatus;
    void checkSocket();

    ConfirmationWindow *_confirmDropItem;
    // Show a confirmation window, then drop item if confirmed
    void dropItemOnConfirmation(size_t serial, size_t slot, const ClientItem *item);


    // Searches
    const ParticleProfile *findParticleProfile(const std::string &id);
    const SoundProfile *findSoundProfile(const std::string &id) const;


    friend class ContainerGrid; // Needs to send CL_SWAP_ITEMS messages, and open a confirmation window
    friend class ClientCombatant;
    friend class ClientObject;
    friend class ClientItem;
    friend class ClientNPC;
    friend class ClientObjectType;
    friend class ClientNPCType;
    friend class ClientVehicle;
    friend void LogSDL::operator()(const std::string &message, const Color &color);
    friend class ItemSelector;
    friend void Window::hideWindow(void *window);
    friend class TakeContainer;
    friend class TestClient;
    friend class Avatar;
    friend class SoundProfile;
    friend class Target;
    friend class ConfirmationWindow;
    friend class Window;
};

#endif
