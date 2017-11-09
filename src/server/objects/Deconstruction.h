#ifndef DECONSTRUCTION_H
#define DECONSTRUCTION_H

#include "../../types.h"

class Deconstruction;
class Object;
class ServerItem;

class DeconstructionType{
public:
    static DeconstructionType *ItemAndTime(const ServerItem *item, ms_t timeToDeconstruct);
    
private:
    DeconstructionType(const ServerItem *item, ms_t timeToDeconstruct);

    const ServerItem *_itemThisBecomes = nullptr;
    ms_t _timeToDeconstruct = 0;

    friend class Deconstruction;
};


class Deconstruction{
public:
    Deconstruction() {}
    Deconstruction(Object &parent, const DeconstructionType &type);

    const ServerItem *becomes() const { return _type->_itemThisBecomes; }
    ms_t timeToDeconstruct() const { return _type->_timeToDeconstruct; }
    bool exists() const { return _type != nullptr; }

private:
    const DeconstructionType *_type;
    Object *_parent = nullptr;

    friend class DeconstructionType;
};

#endif
