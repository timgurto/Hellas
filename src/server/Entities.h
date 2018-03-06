#ifndef ENTITIES_H
#define ENTITIES_H

#include "Entity.h"

class Object;
class Vehicle;

class Entities{
private:
    typedef std::set<Entity *, Entity::compareSerial> Container;
    typedef Container::const_iterator iterator;

public:
    void clear() { _container.clear(); }
    size_t size() const { return _container.size(); }
    bool empty() const { return size() == 0; }
    void insert(Entity *p) { _container.insert(p); }
    size_t erase(Entity *p) { return _container.erase(p); }
    iterator begin() const { return _container.begin(); }
    iterator end() const { return _container.end(); }
    
    Entity *find(size_t serial);
    template<typename T>
    T *find(size_t serial){
        Entity *pEnt = find(serial);
        return dynamic_cast<T *>(pEnt);
    }

    const Vehicle *findVehicleDrivenBy(const User &driver);

private:
    Container _container;
};

#endif
