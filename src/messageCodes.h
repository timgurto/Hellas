// (C) 2015 Tim Gurto

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
    CL_CONSTRUCT = 22,

    // "I want to pick up an object"
    // Arguments: serial
    CL_GATHER = 23,

    // "I want to deconstruct an object"
    // Arguments: serial
    CL_DECONSTRUCT = 24,

    // "I want to trade using merchant slot ... in object ..."
    // Arguments: serial, slot
    CL_TRADE = 25,

    // "I want to drop the item in object ...'s slot ..."
    // An object serial of 0 denotes the user's inventory.
    // Arguments: serial, slot
    CL_DROP = 30,

    // "I want to swap the items in container slots ... and ...". 
    // An object serial of 0 denotes the user's inventory.
    // Arguments: serial1, slot1, serial2, slot2
    CL_SWAP_ITEMS = 31,

    // "I want to set object ...'s merchant slot ... to the following:
    // Sell ...x... for ...x..."
    // Arguments: serial, slot, ware, wareQty, price, priceQty
    CL_SET_MERCHANT_SLOT = 32,

    // "I want to clear object ...'s merchant slot ..."
    // Arguments: serial, slot
    CL_CLEAR_MERCHANT_SLOT = 33,

    // "Tell me what's inside object ..., and let me know of changes in the future". 
    // Arguments: serial
    CL_START_WATCHING = 40,

    // "I'm no longer interested in updates from object ...". 
    // Arguments: serial
    CL_STOP_WATCHING = 41,

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

    // The map size
    // Arguments: x, y
    SV_MAP_SIZE = 120,

    // Terrain details
    // Arguments: starting x, starting y, number in row[, terrain ID] * n
    // e.g. 10, 10, 3, 0, 0, 0 for co-ordinates (10,10) to (13,10)
    SV_TERRAIN = 121,

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

    // The user has begun an action
    // Arguments: time
    SV_ACTION_STARTED = 130,

    // The user has completed an action
    SV_ACTION_FINISHED = 131,

    // An object has an owner
    // Arguments: serial, owner
    SV_OWNER = 150,

    // The user's health value
    // Arguments: hp
    SV_HEALTH = 160,

    // "User ... has said ...".
    // Arguments: username, message
    SV_SAY = 200,

    // "User ... has said ... to you".
    // Arguments: username, message
    SV_WHISPER = 201,

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
    // Arguments: requiredItemClass
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



    NO_CODE
};

#endif