#pragma once

#include <map>

class Entity;
class NPC;

using Threat = int;

class ThreatTable {
 public:
  ThreatTable(const NPC &owner) : _owner(owner) {}

  void makeAwareOf(Entity &entity);
  bool isAwareOf(Entity &entity) const;
  void forgetAbout(const Entity &entity);
  void forgetCurrentTarget();
  void addThreat(Entity &entity, Threat amount);
  void scaleThreat(Entity &entity, double multiplier);
  Entity *getTarget();  // nullptr if table is empty
  void clear();
  bool isEmpty() const;
  size_t size() const;

 private:
  using Container = std::map<Entity *, Threat>;
  Container _container;
  const NPC &_owner;
};
