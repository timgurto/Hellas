#ifndef MESSAGECODES_H
#define MESSAGECODES_H

enum MessageCode{
    // Client -> server
    CL_MOVE_UP,
    CL_MOVE_DOWN,
    CL_MOVE_LEFT,
    CL_MOVE_RIGHT,
    CL_I_AM, // Player name announcement; required before other messages from this client will be accepted

    // Server -> client
    SV_LOCATION,
    SV_OTHER_LOCATION,

    NUM_MESSAGE_CODES
};

#endif