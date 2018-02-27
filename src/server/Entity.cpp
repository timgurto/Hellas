#include <algorithm>
#include <cassert>

#include "Entity.h"
#include "Server.h"
#include "Spawner.h"
#include "../util.h"

const px_t Entity::MELEE_RANGE = Podes{ 4 }.toPixels();

Stats Dummy::_stats{};

Entity::Entity(const EntityType *type, const MapPoint &loc):
    _type(type),
    _serial(generateSerial()),
    _spawner(nullptr),

    _location(loc),
    _lastLocUpdate(SDL_GetTicks()),

    _stats(type->baseStats()),
    _health(_stats.maxHealth),
    _energy(_stats.maxEnergy),

    _attackTimer(0),
    _target(nullptr),
    _loot(nullptr)
{}

Entity::Entity(size_t serial): // For set/map lookup ONLY
    _type(nullptr),
    _serial(serial),
    _spawner(nullptr),
    _loot(nullptr)
{}

Entity::Entity(const MapPoint &loc): // For set/map lookup ONLY
    _type(nullptr),
    _location(loc),
    _serial(0),
    _spawner(nullptr),
    _loot(nullptr)
{}

Entity::~Entity(){
    if (_spawner != nullptr)
        _spawner->scheduleSpawn();
}

bool Entity::compareSerial::operator()( const Entity *a, const Entity *b) const{
    return a->_serial < b->_serial;
}

bool Entity::compareXThenSerial::operator()( const Entity *a, const Entity *b) const{
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_serial < b->_serial;
}

bool Entity::compareYThenSerial::operator()( const Entity *a, const Entity *b) const{
    if (a->_location.y != b->_location.y)
        return a->_location.y < b->_location.y;
    return a->_serial < b->_serial;
}

size_t Entity::generateSerial() {
    static size_t currentSerial = Server::STARTING_SERIAL;
    return currentSerial++;
}

void Entity::markForRemoval(){
    Server::_instance->_entitiesToRemove.push_back(this);
}

bool Entity::combatTypeCanHaveOutcome(CombatType type, CombatResult outcome, SpellSchool school,
        px_t range) {
    /*
                Miss    Dodge   Block   Crit    Hit
    Spell               X       X
    Physical
    Ranged              X
    Heal        X       X       X
    Debuff              X       X       X
    */
    if (type == HEAL && outcome == MISS)
        return false;
    if (type == DEBUFF && outcome != CRIT && outcome != HIT)
        return false;
    if (outcome == DODGE && range > Podes::MELEE_RANGE)
        return false;
    if (outcome == BLOCK && school.isMagic())
        return false;
    if (type == THREAT_MOD && (outcome == BLOCK || outcome == DODGE))
        return false;

    return true;
}

void Entity::sendGotHitMessageTo(const User & user) const {
    Server::_instance->sendMessage(user.socket(), SV_ENTITY_WAS_HIT, makeArgs(serial()));
}

void Entity::reduceHealth(int damage) {
    if (damage == 0)
        return;
    if (damage >= static_cast<int>(_health)) {
        _health = 0;
        onHealthChange();
        onDeath();
    } else {
        _health -= damage;
        onHealthChange();
    }
    broadcastDamagedMessage(damage);

    assert(_health <= this->_stats.maxHealth);
}

void Entity::reduceEnergy(int amount) {
    if (amount == 0)
        return;
    if (amount > static_cast<int>(_energy))
        amount = _energy;
    _energy -= amount;
    onEnergyChange();
}

void Entity::healBy(Hitpoints amount) {
    auto newHealth = min(health() + amount, _stats.maxHealth);
    _health = newHealth;
    onHealthChange();
    broadcastHealedMessage(amount);
}

void Entity::update(ms_t timeElapsed){
    // Corpse timer
    if (isDead()){
        if (_corpseTime > timeElapsed)
            _corpseTime -= timeElapsed;
        else
            markForRemoval();
        return;
    }

    regen(timeElapsed);

    updateBuffs(timeElapsed);

    // The remainder of this function deals with combat.
    if (_attackTimer > timeElapsed)
        _attackTimer -= timeElapsed;
    else
        _attackTimer = 0;

    if (isStunned())
        return;

    auto pTarget = target();
    if (!pTarget)
        return;
    if (!isAttackingTarget())
        return;
    if (_attackTimer > 0)
        return;

    if (pTarget->isDead())
        return;

    if (!canAttack())
        return;

    // Check if within range
    if (distance(collisionRect(), pTarget->collisionRect()) > attackRange())
        return;

    resetAttackTimer();

    onAttack();

    const Server &server = Server::instance();
    MapPoint locus = midpoint(location(), pTarget->location());

    auto outcome = generateHitAgainst(*pTarget, DAMAGE, school(), attackRange());

    auto usersToInform = server.findUsersInArea(locus);
    auto targetLoc = makeArgs(pTarget->location().x, pTarget->location().y);

    switch (outcome) {
    // These cases return
    case MISS:
        for (auto user : usersToInform) {
            server.sendMessage(user->socket(), SV_SHOW_MISS_AT, targetLoc);
            if (attackRange() > MELEE_RANGE)
                sendRangedMissMessageTo(*user);
        }
        return;
    case DODGE:
        for (auto user : usersToInform) {
            server.sendMessage(user->socket(), SV_SHOW_DODGE_AT, targetLoc);
            if (attackRange() > MELEE_RANGE)
                sendRangedMissMessageTo(*user);
        }
        return;

    // These cases continue on
    case CRIT:
        for (auto user : usersToInform)
            server.sendMessage(user->socket(), SV_SHOW_CRIT_AT, targetLoc);
        break;
    case BLOCK:
        for (auto user : usersToInform)
            server.sendMessage(user->socket(), SV_SHOW_BLOCK_AT, targetLoc);
        break;
    }

    // Send ranged message if hit.  This tells the client to create a projectile.  This must be done
    // before the damage, because if the damage kills the target and the attacker subsequently
    // untargets it, then this message will not be sent.
    if (attackRange() > MELEE_RANGE)
        for (auto user : usersToInform)
            sendRangedHitMessageTo(*user);

    // Actually do the damage.
    auto rawDamage = static_cast<double>(_stats.attack);
    if (outcome == CRIT)
        rawDamage *= 2;

    auto resistance = pTarget->_stats.resistanceByType(school());
    auto resistanceMultiplier = (100 - resistance) / 100.0;
    rawDamage *= resistanceMultiplier;

    auto damage = SpellEffect::chooseRandomSpellMagnitude(rawDamage);

    if (outcome == BLOCK) {
        if (_stats.blockValue >= damage)
            damage = 0;
        else
            damage -= _stats.blockValue;
    }

    pTarget->reduceHealth(damage);

    // Alert nearby clients.  This must be done after the damage, so that the client knows whether
    // to play a hit sound or a death sound.
    MessageCode msgCode;
    std::string args;
    char
        attackerTag = classTag(),
        defenderTag = pTarget->classTag();
    if (attackerTag == 'u' && defenderTag != 'u'){
        msgCode = SV_PLAYER_HIT_ENTITY;
        args = makeArgs(
                dynamic_cast<const User *>(this)->name(),
                pTarget->serial());
    } else if (attackerTag != 'u' && defenderTag == 'u'){
        msgCode = SV_ENTITY_HIT_PLAYER;
        args = makeArgs(
                serial(),
                dynamic_cast<const User *>(pTarget)->name());
    } else if (attackerTag == 'u' && defenderTag == 'u') {
        msgCode = SV_PLAYER_HIT_PLAYER;
        args = makeArgs(
                dynamic_cast<const User *>(this)->name(),
                dynamic_cast<const User *>(pTarget)->name());
    } else {
        assert(false);
    }
    for (auto user : usersToInform)
        server.sendMessage(user->socket(), msgCode, args);

    // Give target opportunity to react
    pTarget->onAttackedBy(*this, damage);
}

void Entity::updateBuffs(ms_t timeElapsed) {
    auto aBuffHasExpired = false;
    for (auto i = 0; i != _buffs.size(); ) {
        auto &buff = _buffs[i];
        buff.update(timeElapsed);
        if (buff.hasExpired()) {
            sendLostBuffMsg(buff.type());
            _buffs.erase(_buffs.begin() + i);
            aBuffHasExpired = true;
        } else
            ++i;
    }
    for (auto i = 0; i != _debuffs.size(); ) {
        auto &debuff = _debuffs[i];
        debuff.update(timeElapsed);
        if (debuff.hasExpired()) {
            sendLostDebuffMsg(debuff.type());
            _debuffs.erase(_debuffs.begin() + i);
            aBuffHasExpired = true;
        } else
            ++i;
    }
    if (aBuffHasExpired)
        updateStats();
}

void Entity::onDeath(){
    if (timeToRemainAsCorpse() == 0)
        markForRemoval();
    else
        startCorpseTimer();
}

void Entity::onAttackedBy(Entity &attacker, Threat threat) {
    for (const auto *buff : onHitBuffsAndDebuffs()) {
        buff->proc(&attacker);
    }

    if (isDead())
        attacker.onKilled(*this);
}

void Entity::startCorpseTimer(){
    _corpseTime = timeToRemainAsCorpse();
}

void Entity::addWatcher(const std::string &username){
    _watchers.insert(username);
    Server::debug() << username << " is now watching " << type()->id() << Log::endl;
}

void Entity::removeWatcher(const std::string &username){
    auto erased = _watchers.erase(username);
    if (erased > 0)
        Server::debug() << username << " is no longer watching " << type()->id() << Log::endl;
}

void Entity::location(const MapPoint & newLoc, bool firstInsertion) {
    Server &server = *Server::_instance;

    const User *selfAsUser = nullptr;
    if (classTag() == 'u')
        selfAsUser = dynamic_cast<const User *>(this);

    MapPoint oldLoc = _location;

    auto xChanged = newLoc.x != oldLoc.x;
    auto yChanged = newLoc.y != oldLoc.y;

    if (!firstInsertion) {

        // Remove from location-indexed trees
        assert(server._entitiesByX.size() == server._entitiesByY.size());
        if (classTag() == 'u') {
            if (xChanged) {
                auto numRemoved = server._usersByX.erase(selfAsUser);
                assert(numRemoved == 1);
            }
            if (yChanged) {
                auto numRemoved = server._usersByY.erase(selfAsUser);
                assert(numRemoved == 1);
            }
        }
        if (xChanged) {
            auto numRemoved = server._entitiesByX.erase(this);
            assert(numRemoved == 1);
        }
        if (yChanged) {
            auto numRemoved = server._entitiesByY.erase(this);
            assert(numRemoved == 1);
        }
    }

    _location = newLoc;

    // Re-insert into location-indexed trees
    if (classTag() == 'u') {
        if (xChanged)
            server._usersByX.insert(selfAsUser);
        if (yChanged)
            server._usersByY.insert(selfAsUser);
        assert(server._usersByX.size() == server._usersByY.size());
    }
    if (xChanged)
        server._entitiesByX.insert(this);
    if (yChanged)
        server._entitiesByY.insert(this);
    assert(server._entitiesByX.size() == server._entitiesByY.size());

    // Move to a different collision chunk if needed
    auto
        &oldCollisionChunk = server.getCollisionChunk(oldLoc),
        &newCollisionChunk = server.getCollisionChunk(_location);
    if (firstInsertion || &oldCollisionChunk != &newCollisionChunk) {
        oldCollisionChunk.removeEntity(_serial);
        newCollisionChunk.addEntity(this);
    }
}

std::vector<const Buff*> Entity::onHitBuffsAndDebuffs() {
    auto v = std::vector<const Buff*>{};
    for (const auto &buff : _buffs)
        if (buff.hasEffectOnHit())
            v.push_back(&buff);
    for (const auto &debuff : _debuffs)
        if (debuff.hasEffectOnHit())
            v.push_back(&debuff);
    return v;
}

void Entity::applyBuff(const BuffType & type, Entity &caster) {
    auto newBuff = Buff{ type, *this, caster };

    // Check for duplicates
    for (auto &buff : _buffs)
        if (buff == newBuff)
            return;

    _buffs.push_back(newBuff);
    updateStats();

    sendBuffMsg(type.id());
}

void Entity::applyDebuff(const BuffType & type, Entity &caster) {
    auto newDebuff = Buff{ type, *this, caster };

    // Check for duplicates
    for (auto &debuff : _debuffs)
        if (debuff == newDebuff)
            return;

    _debuffs.push_back(newDebuff);
    updateStats();

    sendDebuffMsg(type.id());
}

void Entity::loadBuff(const BuffType & type, ms_t timeRemaining) {
    auto newBuff = Buff{ type, *this, timeRemaining };

    // Check for duplicates
    for (auto &buff : _buffs)
        if (buff == newBuff)
            return;

    _buffs.push_back(newBuff);
    updateStats();

    sendBuffMsg(type.id());
}

void Entity::loadDebuff(const BuffType & type, ms_t timeRemaining) {
    auto newDebuff = Buff{ type, *this, timeRemaining };

    // Check for duplicates
    for (auto &debuff : _debuffs)
        if (debuff == newDebuff)
            return;

    _debuffs.push_back(newDebuff);
    updateStats();

    sendDebuffMsg(type.id());
}

void Entity::removeDebuff(Buff::ID id) {
    for (auto it = _debuffs.begin(); it != _debuffs.end(); ++it)
        if (it->type() == id) {
            _debuffs.erase(it);
            updateStats();
            // TODO: Alert client that debuff was removed
            return;
        }
}

void Entity::sendBuffMsg(const Buff::ID &buff) const {
    const Server &server = Server::instance();
    server.broadcastToArea(_location, SV_ENTITY_GOT_BUFF, makeArgs(_serial, buff));
}

void Entity::sendDebuffMsg(const Buff::ID &buff) const {
    const Server &server = Server::instance();
    server.broadcastToArea(_location, SV_ENTITY_GOT_DEBUFF, makeArgs(_serial, buff));
}

void Entity::sendLostBuffMsg(const Buff::ID &buff) const {
    const Server &server = Server::instance();
    server.broadcastToArea(_location, SV_ENTITY_LOST_DEBUFF, makeArgs(_serial, buff));
}

void Entity::sendLostDebuffMsg(const Buff::ID &buff) const {
    const Server &server = Server::instance();
    server.broadcastToArea(_location, SV_ENTITY_LOST_DEBUFF, makeArgs(_serial, buff));
}

void Entity::regen(ms_t timeElapsed) {
    // Regen
    _timeSinceRegen += timeElapsed;
    if (_timeSinceRegen < 1000)
        return;

    _timeSinceRegen -= 1000;

    if (stats().hps != 0) {
        int rawNewHealth = health() + stats().hps;
        if (rawNewHealth < 0)
            health(0);
        else if (0 + rawNewHealth > static_cast<int>(stats().maxHealth) + 0)
            health(stats().maxHealth);
        else
            health(rawNewHealth);
        onHealthChange();
        if (isDead())
            onDeath();
    }

    if (stats().eps != 0) {
        int rawNewEnergy = energy() + stats().eps;
        if (rawNewEnergy < 0)
            energy(0);
        else if (rawNewEnergy > static_cast<int>(stats().maxEnergy))
            energy(stats().maxEnergy);
        else
            energy(rawNewEnergy);
        onEnergyChange();
    }
}
