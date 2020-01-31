#include "Object.h"

#include "../../util.h"
#include "../Server.h"
#include "ObjectLoot.h"

Object::Object(const ObjectType *type, const MapPoint &loc)
    : Entity(type, loc),
      QuestNode(*type, serial()),
      _transformTimer(0),
      _disappearTimer(type->disappearsAfter()) {
  setType(type, false, true);
  objType().incrementCounter();

  if (type != &User::OBJECT_TYPE) type->initStrengthAndMaxHealth();
  initStatsFromType();

  _loot.reset(new ObjectLoot(*this));
}

Object::Object(size_t serial) : Entity(serial), QuestNode(QuestNode::Dummy()) {}

Object::Object(const MapPoint &loc)
    : Entity(loc), QuestNode(QuestNode::Dummy()) {}

Object::~Object() {
  if (permissions.hasOwner()) {
    Server &server = *Server::_instance;
    server._objectsByOwner.remove(permissions.owner(), serial());
  }
}

void Object::update(ms_t timeElapsed) {
  if (isBeingBuilt()) return;

  if (_disappearTimer > 0) {
    if (timeElapsed > _disappearTimer)
      markForRemoval();
    else
      _disappearTimer -= timeElapsed;
  }

  // Transform
  do {
    if (isDead()) break;
    if (_transformTimer == 0) break;
    if (objType().transformObject() == nullptr) break;
    if (objType().transformsOnEmpty() && !gatherable.contents().isEmpty())
      break;

    if (timeElapsed > _transformTimer)
      _transformTimer = 0;
    else
      _transformTimer -= timeElapsed;

    if (_transformTimer == 0)
      setType(objType().transformObject(),
              objType().skipConstructionOnTransform());
  } while (false);

  Entity::update(timeElapsed);
}

void Object::onHealthChange() {
  const Server &server = *Server::_instance;
  if (classTag() != 'u')
    for (const User *user : server.findUsersInArea(location()))
      user->sendMessage({SV_ENTITY_HEALTH, makeArgs(serial(), health())});
  Entity::onHealthChange();
}

void Object::onEnergyChange() {
  const Server &server = *Server::_instance;
  if (classTag() != 'u')
    for (const User *user : server.findUsersInArea(location()))
      user->sendMessage({SV_ENTITY_ENERGY, makeArgs(serial(), energy())});
  Entity::onEnergyChange();
}

void Object::setType(const ObjectType *type, bool skipConstruction,
                     bool wasCalledFromConstructor) {
  if (!type) {
    SERVER_ERROR("Trying to set object type to null");
    return;
  }

  Server *server = Server::_instance;

  Entity::type(type);

  gatherable.populateContents();

  if (!wasCalledFromConstructor) {
    server->forceAllToUntarget(*this);
    gatherable.removeAllGatheringUsers();
  }

  delete _container;
  if (objType().hasContainer()) {
    _container = objType().container().instantiate(*this);
  }
  if (objType().hasDeconstruction()) {
    _deconstruction = {*this, objType().deconstruction()};
  }

  if (type->merchantSlots() != 0)
    _merchantSlots = std::vector<MerchantSlot>(type->merchantSlots());

  if (type->transforms()) _transformTimer = type->transformTime();

  if (!skipConstruction) _remainingMaterials = type->materials();

  // Inform nearby users
  if (server != nullptr)
    for (const User *user : server->findUsersInArea(location()))
      sendInfoToClient(*user);
  // Inform owner
  for (const auto &owner : permissions.ownerAsUsernames())
    server->sendMessageIfOnline(
        owner, {SV_OBJECT,
                makeArgs(serial(), location().x, location().y, type->id())});
}

void Object::onDeath() {
  Server &server = *Server::_instance;
  server.forceAllToUntarget(*this);

  populateLoot();

  if (hasContainer()) container().removeAll();

  if (objType().hasOnDestroy()) objType().onDestroy().function(*this);

  if (type() != nullptr) objType().decrementCounter();

  if (permissions.hasOwner() &&
      permissions.owner().type == Permissions::Owner::PLAYER) {
    auto username = permissions.owner().name;
    const auto user = server.getUserByName(username);
    if (user != nullptr) user->onDestroyedOwnedObject(objType());
  }

  Entity::onDeath();
}

bool Object::canBeAttackedBy(const User &user) const {
  if (!permissions.hasOwner()) return false;

  auto type = permissions.owner().type == Permissions::Owner::CITY
                  ? Belligerent::CITY
                  : Belligerent::PLAYER;
  auto asBelligerent = Belligerent{permissions.owner().name, type};

  const auto &server = Server::instance();
  return server._wars.isAtWar(asBelligerent, {user.name()});
}

void Object::populateLoot() {
  auto &objLoot = static_cast<ObjectLoot &>(*_loot);
  objLoot.populate();
}

void Object::repair() { healBy(stats().maxHealth); }

bool Object::isAbleToDeconstruct(const User &user) const {
  if (hasContainer()) return _container->isAbleToDeconstruct(user);
  return true;
}

void Object::sendInfoToClient(const User &targetUser) const {
  const Server &server = Server::instance();

  targetUser.sendMessage({SV_OBJECT, makeArgs(serial(), location().x,
                                              location().y, type()->id())});

  // Owner
  if (permissions.hasOwner()) {
    const auto &owner = permissions.owner();
    targetUser.sendMessage(
        {SV_OWNER, makeArgs(serial(), owner.typeString(), owner.name)});

    // In case the owner is unknown to the client, tell him the owner's city
    if (owner.type == owner.PLAYER) {
      std::string ownersCity = server.cities().getPlayerCity(owner.name);
      if (!ownersCity.empty())
        targetUser.sendMessage({SV_IN_CITY, makeArgs(owner.name, ownersCity)});
    }
  }

  // Being gathered
  if (gatherable.numUsersGathering() > 0)
    targetUser.sendMessage({SV_GATHERING_OBJECT, serial()});

  // Construction materials
  if (isBeingBuilt()) {
    server.sendConstructionMaterialsMessage(targetUser, *this);
  }

  // Transform timer
  if (isTransforming()) {
    targetUser.sendMessage(
        {SV_TRANSFORM_TIME, makeArgs(serial(), transformTimer())});
  }

  // Hitpoints
  if (health() < stats().maxHealth)
    targetUser.sendMessage({SV_ENTITY_HEALTH, makeArgs(serial(), health())});

  // Lootable
  if (_loot != nullptr && !_loot->empty()) sendAllLootToTagger();

  // Container
  if (hasContainer() && permissions.doesUserHaveAccess(targetUser.name()))
    for (auto i = 0; i != objType().container().slots(); ++i)
      server.sendInventoryMessage(targetUser, i, *this);

  // Merchant slots
  for (auto i = 0; i != _merchantSlots.size(); ++i)
    server.sendMerchantSlotMessage(targetUser, *this, i);

  // Buffs/debuffs
  for (const auto &buff : buffs())
    targetUser.sendMessage(
        {SV_ENTITY_GOT_BUFF, makeArgs(serial(), buff.type())});
  for (const auto &debuff : debuffs())
    targetUser.sendMessage(
        {SV_ENTITY_GOT_DEBUFF, makeArgs(serial(), debuff.type())});

  // Quests
  QuestNode::sendQuestsToUser(targetUser);
}

void Object::tellRelevantUsersAboutInventorySlot(size_t slot) const {
  const Server &server = Server::instance();
  for (const auto *user : server.findUsersInArea(location())) {
    if (!permissions.doesUserHaveAccess(user->name())) continue;

    server.sendInventoryMessage(*user, slot, *this);
  }
}

void Object::tellRelevantUsersAboutMerchantSlot(size_t slot) const {
  // All users are relevant; those with permissions can change the slots, and
  // those without can use them.
  const Server &server = Server::instance();
  for (const auto *user : server.findUsersInArea(location()))
    server.sendMerchantSlotMessage(*user, *this, slot);
}

ServerItem::Slot *Object::getSlotToTakeFromAndSendErrors(size_t slotNum,
                                                         const User &user) {
  const Server &server = Server::instance();
  const Socket &socket = user.socket();

  auto hasLoot = !_loot->empty();
  if (!(hasLoot || hasContainer())) {
    user.sendMessage(ERROR_NO_INVENTORY);
    return nullptr;
  }

  if (!server.isEntityInRange(socket, user, this)) return nullptr;

  if (isBeingBuilt()) {
    user.sendMessage(ERROR_UNDER_CONSTRUCTION);
    return nullptr;
  }

  if (hasLoot) {
    ServerItem::Slot &slot = _loot->at(slotNum);
    if (!slot.first.hasItem()) {
      user.sendMessage(ERROR_EMPTY_SLOT);
      return nullptr;
    }
    return &slot;
  }

  if (!permissions.doesUserHaveAccess(user.name())) {
    user.sendMessage(WARNING_NO_PERMISSION);
    return nullptr;
  }

  if (slotNum >= objType().container().slots()) {
    user.sendMessage(ERROR_INVALID_SLOT);
    return nullptr;
  }

  if (!hasContainer()) {
    SERVER_ERROR("Attempting to fetch slot from entity with no container");
    return nullptr;
  }

  ServerItem::Slot &slot = container().at(slotNum);
  if (!slot.first.hasItem()) {
    user.sendMessage(ERROR_EMPTY_SLOT);
    return nullptr;
  }

  return &slot;
}

Message Object::outOfRangeMessage() const {
  return Message(SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
}

bool Object::shouldAlwaysBeKnownToUser(const User &user) const {
  if (permissions.isOwnedByPlayer(user.name())) return true;
  const Server &server = *Server::_instance;
  const auto &city = server.cities().getPlayerCity(user.name());
  if (!city.empty() && permissions.isOwnedByCity(city)) return true;
  return false;
}

void Object::broadcastDamagedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_OBJECT_DAMAGED, makeArgs(serial(), amount)});
}

void Object::broadcastHealedMessage(Hitpoints amount) const {
  Server &server = *Server::_instance;
  server.broadcastToArea(location(),
                         {SV_OBJECT_HEALED, makeArgs(serial(), amount)});
}
