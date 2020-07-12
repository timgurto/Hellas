#pragma once

#include "Sprite.h"

using namespace std::string_literals;

class SoundProfile;

// A sprite that travels in a straight line before disappearing.
class Projectile : public Sprite {
 public:
  struct Type;

  const std::string &particlesAtEnd() const {
    return projectileType().particlesAtEnd;
  }

  virtual void update(double delta) override;

 private:
  Projectile(const Type &type, const MapPoint &start, const MapPoint &end)
      : Sprite(&type, start), _end(end) {}

  const Type &projectileType() const {
    return *dynamic_cast<const Type *>(this->type());
  }
  double speed() const { return projectileType().speed; }
  void willMiss() { _willMiss = true; }

  MapPoint _end;
  const void *_onReachDestinationArg = nullptr;
  bool _willMiss{false};

  using Tail = std::vector<Sprite *>;
  Tail _tail;

 public:
  struct Type : public SpriteType {
    Type(const std::string &id, const ScreenRect &drawRect)
        : SpriteType(drawRect, "Images/Projectiles/"s + id), id(id) {}
    static Type Dummy(const std::string &id) { return Type{id, {}}; }

    struct ptrCompare {
      bool operator()(const Type *lhs, const Type *rhs) const {
        return lhs->id < rhs->id;
      }
    };

    void sounds(const std::string &profile);
    const SoundProfile *sounds() const { return _sounds; }

    std::string id;
    double speed = 0;
    std::string particlesAtEnd{};
    const SoundProfile *_sounds{nullptr};

    SpriteType _tailType;
    int _tailLength{0};
    int _tailSeparation{0};
    std::string _tailParticles{};
    void tail(const std::string &imageFile, const ScreenRect &drawRect,
              int length, int separation, const std::string &particles);

    void instantiate(Client &client, const MapPoint &start, const MapPoint &end,
                     bool willMiss = false) const;
    friend class Projectile;
  };
};
