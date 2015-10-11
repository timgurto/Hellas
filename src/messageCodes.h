// (C) 2015 Tim Gurto

#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

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

    // "I want to craft ..."
    // Arguments: id
    CL_CRAFT = 21,

    // "I want to construct the item in inventory slot ..., at location ..."
    // Arguments: slot, x, y
    CL_CONSTRUCT = 22,

    // "I want to pick up an object"
    // Arguments: serial
    CL_GATHER = 23,



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

    // An item in the user's inventory
    // Arguments: slot, ID, quantity
    SV_INVENTORY = 123,

    // The details of an object
    // Arguments: serial, x, y, type
    SV_OBJECT = 124,

    // An object has been removed
    // Arguments: serial
    SV_REMOVE_OBJECT = 125,

    // The user has begun an action
    // Arguments: time
    SV_ACTION_STARTED = 130,

    // The user has completed an action
    SV_ACTION_FINISHED = 131,

    // An object has an owner
    // Arguments: serial, owner
    SV_OWNER = 150,

    // The client has attempted to connect with a username already in use
    SV_DUPLICATE_USERNAME = 900,

    // The client has attempted to connect with an invalid username
    SV_INVALID_USERNAME = 901,

    // There is no room for more clients
    SV_SERVER_FULL = 902,

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

    // The user tried to craft an item that cannot be crafted
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



    NO_CODE
};

#endif