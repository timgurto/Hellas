#include <set>

#include "ProgressLock.h"
#include "Server.h"

ProgressLock::locksByType_t ProgressLock::locksByType;
std::set<ProgressLockStaging> ProgressLock::stagedLocks;

ProgressLock::ProgressLock(Type triggerType, Type effectType):
_triggerType(triggerType),
_trigger(nullptr),
_effectType(effectType),
_effect(nullptr)
{}

void ProgressLock::registerStagedLocks(){
    for (const ProgressLockStaging &staged : stagedLocks){
        ProgressLock lock(staged._triggerType, staged._effectType);

        const Server &server = Server::instance();
        switch (staged._triggerType){
            case ITEM:
            case GATHER:
            {
                auto it = server._items.find(staged._trigger);
                if (it != server._items.end())
                    lock._trigger = &*it;
                break;
            }
            case CONSTRUCTION:
                lock._trigger = server.findObjectTypeByName(staged._trigger);
                break;
            case RECIPE:
            {
                auto it = server._recipes.find(staged._trigger);
                if (it != server._recipes.end())
                    lock._trigger = &*it;
                break;
            }
            default:
                ;
        }

        if (lock._trigger == nullptr){
            server._debug << Color::RED << "Invalid progress trigger: '" << staged._trigger << "'" << Log::endl;
            continue;
        }

        switch (staged._effectType){
        case RECIPE:
        {
            auto it = server._recipes.find(staged._effect);
            if (it != server._recipes.end())
                lock._effect = &*it;
            break;
        }
        case CONSTRUCTION:
            lock._effect = server.findObjectTypeByName(staged._effect);
            break;
        default:
            ;
        }

        if (lock._effect == nullptr){
            server._debug << Color::RED << "Invalid progress effect: '" << staged._effect << "'" << Log::endl;
            continue;
        }

        locksByType[lock._triggerType].insert(std::make_pair(lock._trigger, lock));
    }
}

void ProgressLock::triggerUnlocks(User &user, Type triggerType, const void *trigger){
    const locks_t &locks = locksByType[triggerType];
    auto toUnlock = locks.equal_range(trigger);

    std::set<const std::string>
        newRecipes,
        newBuilds;

    for (auto it = toUnlock.first; it != toUnlock.second; ++it){
        const ProgressLock &lock = it->second;
        std::string id;
        switch (lock._effectType){
        case RECIPE:
            id = reinterpret_cast<const Recipe *>(lock._effect)->id();
            newRecipes.insert(id);
            user.addRecipe(id);
            break;
        case CONSTRUCTION:
             id = reinterpret_cast<const ObjectType *>(lock._effect)->id();
            newBuilds.insert(id);
            user.addConstruction(id);
            break;
        default:
            ;
        }
    }

    const Server &server = Server::instance();
            
    if (!newRecipes.empty()){ // New recipes unlocked!
        std::string args = makeArgs(newRecipes.size());
        for (const std::string &id : newRecipes)
            args = makeArgs(args, id);
        server.sendMessage(user.socket(), SV_NEW_RECIPES, args);
    }

    if (!newBuilds.empty()){ // New constructions unlocked!
        std::string args = makeArgs(newBuilds.size());
        for (const std::string &id : newBuilds)
            args = makeArgs(args, id);
        server.sendMessage(user.socket(), SV_NEW_CONSTRUCTIONS, args);
    }
}

// Order doesnt' really matter, as long as it's a proper ordering.
bool ProgressLockStaging::operator<(const ProgressLockStaging &rhs) const{
    if (_triggerType != rhs._triggerType)
        return _triggerType < rhs._triggerType;
    if (_effectType != rhs._effectType)
        return _effectType < rhs._effectType;
    if (_trigger != rhs._trigger)
        return _trigger < rhs._trigger;
    return _effect < rhs._effect;
}
