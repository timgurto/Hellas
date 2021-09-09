#include "ObjectType.h"

#include "../Server.h"

ObjectType::ObjectType(const std::string &id)
    : EntityType(id),
      _numInWorld(0),
      _constructionTime(1000),
      _isUnique(false),
      _isUnbuildable(false),
      _knownByDefault(false),
      _merchantSlots(0),
      _bottomlessMerchant(false),
      _container(nullptr) {
  if (_baseStats.maxHealth == 0) _baseStats.maxHealth = 1;
}

void ObjectType::initialise() const {
  if (_buffGranted) _buffGranted->markAsGrantedByObject();
}

void ObjectType::checkUniquenessInvariant() const {
  if (_isUnique && _numInWorld > 1) {
    Server::debug()("There are more than one of a world-unique object",
                    Color::CHAT_ERROR);
  }
}
