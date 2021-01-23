#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

#include <string>

#include "../HasTags.h"
#include "../Rect.h"
#include "../Stats.h"
#include "../TerrainList.h"
#include "Transformation.h"
#include "Yield.h"

class EntityType : public HasTags {
 public:
  EntityType(const std::string id);
  const std::string &id() const { return _id; }
  virtual char classTag() const = 0;

  // Space
  bool collides() const { return _collides; }
  void collides(bool b) { _collides = b; }
  const MapRect &collisionRect() const { return _collisionRect; }
  void collisionRect(const MapRect &r) {
    _collisionRect = r;
    _collides = true;
  }
  const TerrainList &allowedTerrain() const;
  void allowedTerrain(const std::string &id) {
    _allowedTerrain = TerrainList::findList(id);
  }

  enum Rank { COMMON, ELITE, BOSS };
  Rank rank() const { return _rank; }
  void rank(Rank r) { _rank = r; }

  // Combat
  void baseStats(const Stats &stats) { _baseStats = stats; }
  const Stats &baseStats() const { return _baseStats; }

  bool rewardsXP{true};

  // Other
  Yield yield;
  TransformationType transformation;

 protected:
  mutable Stats _baseStats{};

 private:
  std::string _id;

  bool operator<(const EntityType &rhs) const { return _id < rhs._id; }

  // Space
  bool _collides;  // false by default; true if any collisionRect is specified.
  MapRect _collisionRect;  // Relative to position

  const TerrainList *_allowedTerrain;
  Rank _rank{COMMON};
};

#endif
