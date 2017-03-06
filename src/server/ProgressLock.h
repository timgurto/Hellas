#ifndef PROGRESS_LOCK_H
#define PROGRESS_LOCK_H

#include <map>
#include <set>

class ProgressLockStaging;
class User;

// Defines a trigger-effect pair, used to unlock content per user.
class ProgressLock{
public:
    enum Type{          //  As trigger      As effect
                        //  ----------      ---------
        RECIPE,         //  Crafted         Can craft
        CONSTRUCTION,   //  Built           Can build
        ITEM,           //  Acquired        -
        GATHER,         //  Gathered        -
    };
    typedef std::multimap<const void *, ProgressLock> locks_t; // trigger -> lock
    typedef std::map<Type, locks_t> locksByType_t;

private:
    Type
        _triggerType,
        _effectType;
    const void
        *_trigger,
        *_effect;

    static locksByType_t locksByType;
    static std::set<ProgressLockStaging> stagedLocks;
    
    bool isValid() const { return _trigger != nullptr; }

public:
    ProgressLock(Type triggerType, Type effectType);

    static void registerStagedLocks();
    static void triggerUnlocks(User &user, Type triggerType, const void *trigger);

    friend class ProgressLockStaging;
};

// id strings rather than pointers; assembled during data loading and registered at once afterwards.
class ProgressLockStaging{
    ProgressLock::Type
        _triggerType,
        _effectType;
    std::string
        _trigger,
        _effect;

public:
    ProgressLockStaging(ProgressLock::Type triggerType, const std::string &trigger,
                        ProgressLock::Type effectType, const std::string &effect):
    _triggerType(triggerType), _trigger(trigger), _effectType(effectType), _effect(effect){}
    void stage() const { ProgressLock::stagedLocks.insert(*this); }

    bool operator<(const ProgressLockStaging &rhs) const;

    friend class ProgressLock;
};
#endif
