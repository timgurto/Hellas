#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

enum MessageCode{

    // Client -> server

    // A ping, to measure latency and reassure the server
    // Arguments: time sent
    CL_PING,

    // "My location has changed, and is now ..."
    // Arguments: x, y
    CL_LOCATION,

    // "My name is ..."
    // This has the effect of registering the user with the server.
    // Arguments: username
    CL_I_AM,

    // User wants to collect a branch
    // Arguments: serial
    CL_COLLECT_BRANCH,

    // User wants to collect a tree
    // Arguments: serial
    CL_COLLECT_TREE,



    // Server -> client
    
    // A reply to a ping from a client
    // Arguments: time original was sent, time of reply
    SV_PING_REPLY,

    // A user has disconnected.
    // Arguments: username
    SV_USER_DISCONNECTED,

    // The client has attempted to connect with a username already in use
    SV_DUPLICATE_USERNAME,

    // The client has attempted to connect with an invalid username
    SV_INVALID_USERNAME,

    // There is no room for more clients
    SV_SERVER_FULL,

    // The user is too far away to perform an action
    SV_TOO_FAR,

    // The user tried to perform an action on an object that doesn't exist
    SV_DOESNT_EXIST,

    // The user cannot receive an item because his inventory is full
    SV_INVENTORY_FULL,

    // The client has been successfully registered
    SV_WELCOME,

    // The map size
    // Arguments: x, y
    SV_MAP_SIZE,

    // Terrain details
    // Arguments: starting x, starting y, number in row[, terrain ID] * n
    // e.g. 10, 10, 3, 0, 0, 0 for co-ordinates (10,10) to (13,10)
    SV_TERRAIN,

    // The location of a user
    // Arguments: username, x, y
    SV_LOCATION,

    // An item in the user's inventory
    // Arguments: slot, ID, quantity
    SV_INVENTORY,

    // The location of a branch
    // Arguments: serial, x, y
    SV_BRANCH,

    // The location of a tree
    // Arguments: serial, x, y
    SV_TREE,

    // A branch has been removed
    // Arguments: serial
    SV_REMOVE_BRANCH,

    // A tree has been removed
    // Arguments: serial
    SV_REMOVE_TREE,



    NUM_MESSAGE_CODES
};

#endif