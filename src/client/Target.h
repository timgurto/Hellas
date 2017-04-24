#ifndef TARGET_HEADER
#define TARGET_HEADER

class Entity;
class ClientCombatant;

/*
Both pointers should contain the same value.  Having both is necessary because reinterpret_cast
doesn't appear to work.
*/
class Target{
public:
    Target();

    void set(const Entity &asEntity, const ClientCombatant &asCombatant);
    void clear();

    const Entity *entity() const { return _entity; }
    const ClientCombatant *combatant() const { return _combatant; }
    bool exists() const { return _entity != nullptr; }

private:
    const Entity *_entity;
    const ClientCombatant *_combatant;
};

#endif
