#ifndef PROGRESS_LOCK_H
#define PROGRESS_LOCK_H

#include <map>
#include <set>
#include <string>

class ProgressLockStaging;
class User;

// Defines a trigger-effect pair, used to unlock content per user.
class ProgressLock {
 public:
  enum Type {      //  As trigger      As effect
                   //  ----------      ---------
    RECIPE,        //  Crafted         Can craft
    CONSTRUCTION,  //  Built           Can build
    ITEM,          //  Acquired        -
    GATHER,        //  Gathered        -
  };
  typedef std::multimap<const void *, ProgressLock> locks_t;  // trigger -> lock
  typedef std::map<Type, locks_t> locksByType_t;

 private:
  Type _triggerType, _effectType;
  const void *_trigger, *_effect;
  std::string _triggerID, _effectID;
  double _chance;  // The chance that the trigger actually causes the effect.
                   // (0, 1]

  static locksByType_t locksByType;
  static std::set<ProgressLock> stagedLocks;

  bool isValid() const { return _trigger != nullptr; }

 public:
  ProgressLock(Type triggerType, const std::string &triggerID, Type effectType,
               const std::string &effectID, double chance);

  static void registerStagedLocks();
  static void triggerUnlocks(User &user, Type triggerType, const void *trigger);
  static void unlockAll(User &user);
  void stage() const { stagedLocks.insert(*this); }

  static void cleanup();

  bool ProgressLock::operator<(const ProgressLock &rhs) const;
};
#endif
