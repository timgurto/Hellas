#pragma once

class Entity;

class EntityComponent {
 public:
  EntityComponent(Entity& parent) : _parent(parent) {}

 protected:
  Entity& parent() const { return _parent; }

 private:
  Entity& _parent;
};
