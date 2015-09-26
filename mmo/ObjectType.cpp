// (C) 2015 Tim Gurto

#include "ObjectType.h"

ObjectType::ObjectType(const std::string &id, Uint32 actionTime, size_t wood,
                       const std::string &gatherReq):
_id(id),
_actionTime(actionTime),
_wood(wood),
_gatherReq(gatherReq){}
