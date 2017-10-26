#pragma once

#include "Sprite.h"

// A sprite that travels in a straight line before disappearing.
class Projectile : public Sprite {
public:
    class Type;

    Projectile(const Type &type, const Point &start, const Point &end) :
        Sprite(&type, start), _end(end) {}

    virtual void update(double delta) override;

private:
    const Type &spriteType() const { return * dynamic_cast<const Type *>(this->type()); }
    double speed() const { return spriteType().speed(); }

    Point _end;


public:
    class Type : public SpriteType {
    public:
        Type(const std::string &id, double speed, const Rect &drawRect, const std::string &imageFile) :
            SpriteType(drawRect, imageFile), _id(id), _speed(speed) {
        }
        static Type Dummy(const std::string &id) { return Type{ id, 0, {}, {} }; }

        struct ptrCompare {
            bool operator()(const Type *lhs, const Type *rhs) const {
                return lhs->_id < rhs->_id;
            }
        };

        double speed() const { return _speed; }

    private:
        std::string _id;
        double _speed;
    };
};
