#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

enum MessageCode{

    // Client -> server

    // "My location has changed, and is now ..."
    // Arguments: x, y
    CL_LOCATION,

    // "My name is ..."
    // This has the effect of registering the user with the server.
    // Arguments: username
    CL_I_AM,


    // Server -> client

    // The location of a user.
    // Arguments: username, x, y
    SV_LOCATION,

    // A user has disconnected.
    // Arguments: username
    SV_USER_DISCONNECTED,

    // The client has attempted to connect with a username already in use
    SV_DUPLICATE_USERNAME,

    // The client has attempted to connect with an invalid username
    SV_INVALID_USERNAME,

    NUM_MESSAGE_CODES
};

#endif