#include "Server.h"
#include "Spawner.h"

Spawner::Spawner(size_t index, const Point &location, const ObjectType *type):
    _index(index),
    _location(location),
    _type(type),

    _radius(0),
    _quantity(1),
    _respawnTime(0){}

void Spawner::spawn(){
    static const size_t MAX_ATTEMPTS = 50;
    Server &server = *Server::_instance;

    for (size_t attempt = 0; attempt != MAX_ATTEMPTS; ++attempt){

        // Choose location
        Point p = _location;
        // Random point in circle
        if (_radius != 0){
            double radius = sqrt(randDouble()) * _radius;
            double angle = randDouble() * 2 * PI;
            p.x += cos(angle) * radius;
            p.y -= sin(angle) * radius;
        }

        // Check terrain whitelist
        if (!_terrainWhitelist.empty()){
            char terrain = server.findTile(p);
            if (_terrainWhitelist.find(terrain) == _terrainWhitelist.end())
                continue;
        }

        // Check location validity
        if (!server.isLocationValid(p, *_type))
            continue;

        // Add object;
        Entity *entity;
        if (_type->classTag() == 'n')
            entity = &server.addNPC(dynamic_cast<const NPCType *>(_type), p);
        else
            entity = &server.addObject(dynamic_cast<const ObjectType *>(_type), p);
        entity->spawner(this);
        return;
    }

    server._debug << Color::YELLOW << "Failed to spawn object " << _type->id() << "." << Log::endl;
    scheduleSpawn();
}

void Spawner::scheduleSpawn(){
    Log &d = Server::_instance->_debug;
    _spawnSchedule.push_back(SDL_GetTicks() + _respawnTime);
}

void Spawner::update(ms_t currentTime){
    while (!_spawnSchedule.empty() && _spawnSchedule.front() <= currentTime){
        _spawnSchedule.pop_front();
        spawn();
    }
}
