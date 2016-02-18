// (C) 2015 Tim Gurto

#ifndef CLIENT_H
#define CLIENT_H

#include <queue>
#include <string>
#include <map>
#include <set>

#include "Avatar.h"
#include "ClientObject.h"
#include "ClientObjectType.h"
#include "Entity.h"
#include "Item.h"
#include "LogSDL.h"
#include "ui/ChoiceList.h"
#include "ui/Window.h"
#include "../Args.h"
#include "../Point.h"
#include "../Rect.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../server/Recipe.h"
#include "ui/Container.h"

class Client{
public:
    Client();
    ~Client();
    void run();

    static bool isClient;

    const Socket &socket() const;
    TTF_Font *defaultFont() const;

    const Entity &character() const { return _character; }
    const Point &offset() const { return _intOffset; }
    const std::string &username() const { return _username; }
    const Entity *currentMouseOverEntity() const { return _currentMouseOverEntity; }
    Rect playerCollisionRect() const { return _character.location() +
                                              Avatar::collisionRect(); }

    static const int PLAYER_ACTION_CHANNEL;

    static const int ICON_SIZE;
    static const int TILE_W, TILE_H;
    static const double MOVEMENT_SPEED;

    static const size_t INVENTORY_SIZE;
    static const int ACTION_DISTANCE;

    static LogSDL &debug() { return *_debugInstance; }

    typedef std::list<Window *> windows_t;

private:
    static LogSDL *_debugInstance;

    static const Uint32 MAX_TICK_LENGTH;
    static const Uint32 SERVER_TIMEOUT; // How long the client will wait for a ping reply
    static const Uint32 CONNECT_RETRY_DELAY; // How long to wait between retries at connecting
    static const Uint32 PING_FREQUENCY; // How often to test latency with each client
    // How often to send location updates to server (while moving)
    static const Uint32 TIME_BETWEEN_LOCATION_UPDATES;

    static const int
        SCREEN_X,
        SCREEN_Y;

    static Client *_instance;

    Texture
        _cursorNormal,
        _cursorGather,
        _cursorContainer;
    const Texture *_currentCursor;

    Texture _invLabel;
    static Rect INVENTORY_RECT; // non-const, as it needs to be initialized at runtime.
    static const size_t ICONS_X; // How many icons per inventory row
    // Whether the user has the specified item(s).
    bool playerHasItem(const Item *item, size_t quantity = 1) const;

    void initializeCraftingWindow();
    bool _haveMatsFilter, _haveToolsFilter, _classOr, _matOr;
    static const int HEADING_HEIGHT; // The height of windows' section headings
    static const int LINE_GAP; // The total height occupied by a line and its surrounding spacing
    const Recipe *_activeRecipe; // The recipe currently selected, if any
    static void startCrafting(void *data); // Called when the "Craft" button is clicked.
    // Populated at load time, after _items
    std::map<std::string, bool> _classFilters;
    std::map<const Item *, bool> _matFilters;
    mutable bool _classFilterSelected, _matFilterSelected; // Whether any filters have been selected
    bool recipeMatchesFilters(const Recipe &recipe) const;
    // Called when filters pane is clicked.
    static void populateRecipesList(Element &e);
    // Called when a recipe is selected.
    static void selectRecipe(Element &e, const Point &mousePos);

    windows_t _windows;
    void addWindow(Window *window);
    void removeWindow(Window *window); // Linear time

    ChoiceList *_recipeList;
    Element *_detailsPane;

    Window *_craftingWindow;

    Window *_inventoryWindow;
    void initializeInventoryWindow();
    Texture _constructionFootprint;

    Texture _tile[5];

    Avatar _character; // Describes the user's character
    Point _pendingCharLoc; // Where the player has told his character to go. Unconfirmed by server.

    // These are superficial, and relate only to the cast bar.
    Uint32 _actionTimer; // How long the character has been performing the current action.
    Uint32 _actionLength; // How long the current action should take.
    std::string _actionMessage; // A description of the current action.
    void prepareAction(const std::string &msg); // Set up the action, awaiting server confirmation.
    void startAction(Uint32 actionLength); // Start the action timer.  If zero, stop the timer.

    bool _loop;
    Socket _socket;
    TTF_Font *_defaultFont;
    int _defaultFontOffset; // Vertical offset for game text
    std::string _username;

    // Mouse stuff
    Point _mouse; // Mouse position
    bool _mouseMoved;
    Entity *getEntityAtMouse();
    void checkMouseOver();

    bool _leftMouseDown; // Whether the left mouse button is pressed
    Entity *_leftMouseDownEntity;
    friend void ClientObject::onLeftClick(Client &client);

    bool _rightMouseDown;
    Entity *_rightMouseDownEntity;
    friend void ClientObject::onRightClick(Client &client);


    void draw() const;
    void drawTile(size_t x, size_t y, int xLoc, int yLoc) const;
    void drawTooltip() const;
    // A tooltip which, if it exists, describes the UI element currently moused over.
    const Texture *_uiTooltip;
    Point _offset; // An offset for drawing, based on the character's location on the map.
    Point _intOffset; // An integer version of the offset
    void updateOffset(); // Update the offset, when the character moves.

    std::string _partialMessage;

    Uint32 _time;
    Uint32 _timeElapsed; // Time between last two ticks
    Uint32 _lastPingReply; // The last time a ping reply was received from the server
    Uint32 _lastPingSent; // The last time a ping was sent to the server
    Uint32 _latency;
    Uint32 _timeSinceConnectAttempt;

    bool _invalidUsername; // Flag set if server refused username

    bool _loggedIn;
    bool _loaded; // Whether the client has sufficient information to begin

    static const size_t BUFFER_SIZE;

    Uint32 _timeSinceLocUpdate; // Time since a CL_LOCATION was sent
    // Location has changed (local or official), and tooltip may have changed.
    bool _tooltipNeedsRefresh;
    Texture getInventoryTooltip() const; // Generate tooltip for the inventory

    // Game data
    std::set<Item> _items;
    std::set<Recipe> _recipes;
    std::set<ClientObjectType> _objectTypes;

    // Information about the state of the world
    size_t _mapX, _mapY;
    std::vector<std::vector<size_t> > _map;
    Item::vect_t _inventory;
    std::map<std::string, Avatar*> _otherUsers; // For lookup by name
    std::map<size_t, ClientObject*> _objects; // For lookup by serial

    Entity::set_t _entities;
    void removeEntity(Entity *const toRemove); // Remove from _entities, and delete pointer
    // Move the entity, and reorder it if necessary
    void setEntityLocation(Entity *entity, const Point &location);
    Entity *_currentMouseOverEntity;

    std::queue<std::string> _messages;

    mutable LogSDL _debug;

    std::string _enteredText; // Text that has been entered by the user
    static const size_t MAX_TEXT_ENTERED;

    void checkSocket();
    void sendRawMessage(const std::string &args = "") const;
    void sendMessage(MessageCode msgCode, const std::string &args = "") const;
    void handleMessage(const std::string &msg);

    friend class Container; // Needs to send CL_SWAP_ITEMS messages
    friend ClientObject::~ClientObject();
};

#endif
