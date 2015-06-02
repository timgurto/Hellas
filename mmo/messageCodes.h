#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

enum MessageCode{

    // Client -> server

    // A reply to a ping from the server
    // Arguments: time original was sent, time of reply
    CL_PING_REPLY,

    // "My location has changed, and is now ..."
    // Arguments: x, y
    CL_LOCATION,

    // "My name is ..."
    // This has the effect of registering the user with the server.
    // Arguments: username
    CL_I_AM,



    // Server -> client
    
    // A ping, to measure latency and reassure clients
    // Arguments: time sent
    SV_PING,

    // A second ping reply, so that the client can measure latency
    // Arguments: time reply sent from client
    SV_PING_REPLY_2,

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