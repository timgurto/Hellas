#pragma once

#include <map>

class Entity;

using Threat = int;

class ThreatTable {
public:
    void makeAwareOf(Entity &entity);
    void addThreat(Entity &entity, Threat amount);
    Entity *getTarget(); // nullptr if table is empty

private:
    using Container = std::map<Entity *, Threat>;
    Container _container;
};
