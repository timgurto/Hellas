#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

const char
    MSG_START = '\002', // STX
    MSG_END = '\003', // ETX
    MSG_DELIM = '\037'; // US

enum MessageCode{

    // Client -> server

    // A ping, to measure latency and reassure the server
    // Arguments: time sent
    CL_PING = 0,

    // "My name is ..."
    // This has the effect of registering the user with the server.
    // Arguments: username
    CL_I_AM = 1,

    // "My location has changed, and is now ..."
    // Arguments: x, y
    CL_LOCATION = 10,

    // Cancel user's current action
    CL_CANCEL_ACTION = 20,

    // "I want to craft using recipe ..."
    // Arguments: id
    CL_CRAFT = 21,

    // "I want to construct the item in inventory slot ..., at location ..."
    // Arguments: slot, x, y
    CL_CONSTRUCT_ITEM = 22,

    // "I want to construct object ..., at location ..."
    // Arguments: id, x, y
    CL_CONSTRUCT = 23,

    // "I want to pick up an object"
    // Arguments: serial
    CL_GATHER = 24,

    // "I want to deconstruct an object"
    // Arguments: serial
    CL_DECONSTRUCT = 25,

    // "I want to trade using merchant slot ... in object ..."
    // Arguments: serial, slot
    CL_TRADE = 26,

    // "I want to drop the item in object ...'s slot ..."
    // An object serial of 0 denotes the user's inventory.
    // Arguments: serial, slot
    CL_DROP = 30,

    // "I want to swap the items in container slots ... and ...". 
    // An object serial of 0 denotes the user's inventory.
    // An object serial of 1 denotes the user's gear.
    // Arguments: serial1, slot1, serial2, slot2
    CL_SWAP_ITEMS = 31,

    // "I want to take the item in container slot ..."
    // An object serial of 1 denotes the user's gear.
    // Arguments: serial, slot
    CL_TAKE_ITEM = 32,

    // "I want to set object ...'s merchant slot ... to the following:
    // Sell ...x... for ...x..."
    // Arguments: serial, slot, ware, wareQty, price, priceQty
    CL_SET_MERCHANT_SLOT = 33,

    // "I want to clear object ...'s merchant slot ..."
    // Arguments: serial, slot
    CL_CLEAR_MERCHANT_SLOT = 34,

    // "I want to mount vehicle ..."
    // Arguments: serial
    CL_MOUNT = 35,

    // "I want to dismount my vehicle, to location (..., ...)"
    // Arguments: x, y
    CL_DISMOUNT = 36,

    // "Tell me what's inside object ..., and let me know of changes in the future". 
    // Arguments: serial
    CL_START_WATCHING = 40,

    // "I'm no longer interested in updates from object ...". 
    // Arguments: serial
    CL_STOP_WATCHING = 41,

    // "I want to give my object ... to my city". 
    // Arguments: serial
    CL_CEDE = 42,

    // "I'm targeting NPC ..."
    // Arguments: serial
    CL_TARGET_NPC = 50,

    // "I'm targeting player ..."
    // Arguments: username
    CL_TARGET_PLAYER = 51,

    // "I want to declare war on ..."
    // Arguments: username
    CL_DECLARE_WAR = 55,

    // "I want to say ... to everybody". 
    // Arguments: message
    CL_SAY = 60,

    // "I want to say ... to ...". 
    // Arguments: username, message
    CL_WHISPER = 61,



    // Server -> client
    
    // A reply to a ping from a client
    // Arguments: time original was sent, time of reply
    SV_PING_REPLY = 100,

    // The client has been successfully registered
    SV_WELCOME = 101,

    // A user has disconnected.
    // Arguments: username
    SV_USER_DISCONNECTED = 110,

    // A user has moved far away from you, and you will stop getting updates from him.
    // Arguments: username
    SV_USER_OUT_OF_RANGE = 111,

    // An object has moved far away from you, and you will stop getting updates from him.
    // Arguments: serial
    SV_OBJECT_OUT_OF_RANGE = 112,

    // The location of a user
    // Arguments: username, x, y
    SV_LOCATION = 122,

    // An item is in the user's inventory, or a container object
    // Arguments: serial, slot, ID, quantity
    SV_INVENTORY = 123,

    // The details of an object
    // Arguments: serial, x, y, type
    SV_OBJECT = 124,

    // An object has been removed
    // Arguments: serial
    SV_REMOVE_OBJECT = 125,

    // Details of an object's merchant slot
    // Arguments: serial, slot, ware, wareQty, price, priceQty
    SV_MERCHANT_SLOT = 126,

    // The health of an NPC
    // Arguments: serial, health
    SV_NPC_HEALTH = 127,

    // The location of an object
    // Arguments: serial, x, y
    SV_OBJECT_LOCATION = 128,

    // An object is transforming
    // Arguments: serial, remaining
    SV_TRANSFORM_TIME = 129,

    // The user has begun an action
    // Arguments: time
    SV_ACTION_STARTED = 130,

    // The user has completed an action
    SV_ACTION_FINISHED = 131,

    // A user's class
    // Arguments: username, classname
    SV_CLASS = 132,

    // A user's gear
    // Arguments: username, slot, id
    SV_GEAR = 133,

    // The recipes a user knows
    // Arguments: quantity, id1, id2, ...
    SV_RECIPES = 134,

    // New recipes a user has just learned
    // Arguments: quantity, id1, id2, ...
    SV_NEW_RECIPES = 135,

    // The constructions a user knows
    // Arguments: quantity, id1, id2, ...
    SV_CONSTRUCTIONS = 136,

    // New constructions a user has just learned
    // Arguments: quantity, id1, id2, ...
    SV_NEW_CONSTRUCTIONS = 137,

    // The user has just joined a city
    // Arguments: cityName
    SV_JOINED_CITY = 138,

    // A user is a member of a city
    // Arguments: username, cityName
    SV_IN_CITY = 139,

    // An NPC hit a player
    // Arguments: serial, username
    SV_NPC_HIT_PLAYER = 140,
    
    // A player hit an NPC
    // Arguments: username, serial
    SV_PLAYER_HIT_NPC = 141,
    
    // A player hit an another player
    // Arguments: attacker's username, defender's username
    SV_PLAYER_HIT_PLAYER = 142,

    // An object has an owner
    // Arguments: serial, type ("user"|"city"), name
    SV_OWNER = 150,

    // An object is being gathered from
    SV_GATHERING_OBJECT = 151,

    // An object is not being gathered from
    SV_NOT_GATHERING_OBJECT = 152,

    // An NPC can be looted
    SV_LOOTABLE = 153,

    // An NPC can no longer be looted
    SV_NOT_LOOTABLE = 154,

    // A vehicle has a driver.
    // Arguments: serial, username
    SV_MOUNTED = 155,

    // A vehicle is no longer being driven.
    // Arguments: serial, username
    SV_UNMOUNTED = 156,

    // The remaining materials required to construct an object
    // Arguments: serial, n, id1, quantity1, id2, quantity2, ...
    SV_CONSTRUCTION_MATERIALS = 157,

    // The user's health value
    // Arguments: hp
    SV_HEALTH = 160,

    // The user's stats
    // Arguments: max health, attack, attack time, speed
    SV_STATS = 161,

    // "You are at war with ..."
    // Arguments: username
    SV_AT_WAR_WITH = 162,

    // "User ... has said ...".
    // Arguments: username, message
    SV_SAY = 200,

    // "User ... has said ... to you".
    // Arguments: username, message
    SV_WHISPER = 201,



    // Errors and warnings

    // The client has attempted to connect with a username already in use
    SV_DUPLICATE_USERNAME = 900,

    // The client has attempted to connect with an invalid username
    SV_INVALID_USERNAME = 901,

    // There is no room for more clients
    SV_SERVER_FULL = 902,

    // That user doesn't exist
    SV_INVALID_USER = 903,

    // The user is too far away to perform an action
    SV_TOO_FAR = 910,

    // The user tried to perform an action on an object that doesn't exist
    SV_DOESNT_EXIST = 911,

    // The user cannot receive an item because his inventory is full
    SV_INVENTORY_FULL = 912,

    // The user does not have enough materials to craft an item
    SV_NEED_MATERIALS = 913,

    // The user tried to craft an item that does not exist
    SV_INVALID_ITEM = 914,

    // The user referred to a nonexistent item
    SV_CANNOT_CRAFT = 915,

    // The user was unable to complete an action
    SV_ACTION_INTERRUPTED = 916,

    // The user tried to manipulate an empty inventory slot
    SV_EMPTY_SLOT = 917,

    // The user attempted to manipulate an out-of-range inventory slot
    SV_INVALID_SLOT = 918,

    // The user tried to construct an item that cannot be constructed
    SV_CANNOT_CONSTRUCT = 919,

    // The user tried to perform an action but does not have the requisite item
    // Arguments: requiredItemTag
    SV_ITEM_NEEDED = 920,

    // The user tried to perform an action at an occupied location
    SV_BLOCKED = 921,

    // The user does not have the tools required to craft an item
    SV_NEED_TOOLS = 922,

    // The user tried to deconstruct an object that cannot be deconstructed
    SV_CANNOT_DECONSTRUCT = 923,

    // The user does not have permission to perform an action
    SV_NO_PERMISSION = 924,

    // The user tried to perform a merchant function on a non-merchant object
    SV_NOT_MERCHANT = 925,

    // The user tried to perform a merchant function on an invalid merchant slot
    SV_INVALID_MERCHANT_SLOT = 926,

    // The merchant has no wares in stock to sell the user
    SV_NO_WARE = 927,

    // The user cannot afford the price of a merchant exchange
    SV_NO_PRICE = 928,

    // The merchant object does not have enough inventory space to trade with the user
    SV_MERCHANT_INVENTORY_FULL = 929,

    // The object cannot be removed because it has an inventory of items
    SV_NOT_EMPTY = 930,

    // The object is not an NPC
    SV_NOT_NPC = 931,

    // The NPC is dead
    SV_NPC_DEAD = 932,

    // The user tried to put an item into an NPC
    SV_NPC_SWAP = 933,

    // The user tried to take an item from himself
    SV_TAKE_SELF = 934,

    // The user tried to equip an item into a gear slot with which it doesn't compatible.
    SV_NOT_GEAR = 935,

    // The user tried to mount a non-vehicle object.
    SV_NOT_VEHICLE = 936,

    // The user tried to perform an action on an occupied vehicle
    SV_VEHICLE_OCCUPIED = 937,

    // The user tried to perform an action on an occupied vehicle
    SV_NO_VEHICLE = 938,

    // The user tried to craft using a recipe he doesn't know
    SV_UNKNOWN_RECIPE = 939,

    // The user tried to construct something that he doesn't know about
    SV_UNKNOWN_CONSTRUCTION = 940,

    // The user tried to add the wrong building material to a site
    SV_WRONG_MATERIAL = 941,

    // The user tried to use an object that is still under construction
    SV_UNDER_CONSTRUCTION = 942,

    // The user tried to attack a player without being at war with him
    SV_AT_PEACE = 943,

    // The user tried to construct a unique object that already exists in the world
    SV_UNIQUE_OBJECT = 944,

    // The user tried to construct an object that cannot be constructed
    SV_UNBUILDABLE = 945,

    // The user tried to construct an object type that doesn't exist
    SV_INVALID_OBJECT = 946,

    // The user tried to declare war on somebody with whom they are already at war.
    SV_ALREADY_AT_WAR = 947,

    // The user tried to perform a city action when not in a city.
    SV_NOT_IN_CITY = 948,



    // Debug requests

    // "Give me a full stack of ..."
    // Arguments: id
    DG_GIVE = 2000,

    // "Unlock everything for me"
    DG_UNLOCK = 2001,



    NO_CODE
};

#endif