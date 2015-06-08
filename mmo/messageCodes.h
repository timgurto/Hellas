#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

enum MessageCode{

    // Client -> server

    // Acknowledge a server message
    // Arguments: serial
    CL_ACK,

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



    // Server -> client
    
    // A reply to a ping from a client
    // Arguments: time original was sent, time of reply
    SV_PING_REPLY,

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

    // There is no room for more clients
    SV_SERVER_FULL,

    // The client has been successfully registered
    SV_WELCOME,



    NUM_MESSAGE_CODES
};

#endif