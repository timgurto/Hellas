#pragma once

#include "Sprite.h"

using namespace std::string_literals;

// A sprite that travels in a straight line before disappearing.
class Projectile : public Sprite {
public:
    struct Type;

    Projectile(const Type &type, const MapPoint &start, const MapPoint &end) :
        Sprite(&type, start), _end(end) {}

    const std::string &particlesAtEnd() const { return projectileType().particlesAtEnd; }

    virtual void update(double delta) override;

private:
    const Type &projectileType() const { return * dynamic_cast<const Type *>(this->type()); }
    double speed() const { return projectileType().speed; }

    MapPoint _end;
    const void *_onReachDestinationArg = nullptr;


public:
    struct Type : public SpriteType {
        Type(const std::string &id, const ScreenRect &drawRect) :
            SpriteType(drawRect, "Images/Projectiles/"s + id + ".png"s), id(id) {
        }
        static Type Dummy(const std::string &id) { return Type{ id, {} }; }

        struct ptrCompare {
            bool operator()(const Type *lhs, const Type *rhs) const {
                return lhs->id < rhs->id;
            }
        };

        std::string id;
        double speed = 0;
        std::string particlesAtEnd = {};
    };
};
