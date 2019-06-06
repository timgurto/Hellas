#pragma once

#include <map>

class Entity;

using Threat = int;

class ThreatTable {
 public:
  void makeAwareOf(Entity &entity);
  bool isAwareOf(Entity &entity) const;
  void forgetAbout(const Entity &entity);
  void addThreat(Entity &entity, Threat amount);
  void scaleThreat(Entity &entity, double multiplier);
  Entity *getTarget();  // nullptr if table is empty
  void clear();
  bool isEmpty() const;
  size_t size() const;

 private:
  using Container = std::map<Entity *, Threat>;
  Container _container;
};
