#ifndef MESSAGECODES_H
#define MESSAGECODES_H

enum MessageCode{
    // Client requests
    REQ_MOVE_UP,
    REQ_MOVE_DOWN,
    REQ_MOVE_LEFT,
    REQ_MOVE_RIGHT,

    //Server messages
    MSG_LOCATION,
    MSG_OTHER_LOCATION,

    NUM_MESSAGE_CODES
};

#endif