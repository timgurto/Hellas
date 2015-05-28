#ifndef MESSAGE_CODES_H
#define MESSAGE_CODES_H

enum MessageCode{

    // Client -> server

    CL_MOVE_UP,
    CL_MOVE_DOWN,
    CL_MOVE_LEFT,
    CL_MOVE_RIGHT,

    // Announcement of a user's name.
    // All client messages are ignored unless its user has been announced with this command.
    // Arguments: username
    CL_I_AM,


    // Server -> client

    // The location of a user.
    // Arguments: username, x, y
    SV_LOCATION,

    NUM_MESSAGE_CODES
};

#endif