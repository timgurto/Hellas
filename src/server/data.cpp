#ifndef SINGLE_THREAD
#include <mutex>
#include <thread>
#endif

#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "DataLoader.h"
#include "DroppedItem.h"
#include "Server.h"
#include "Vehicle.h"

extern Args cmdLineArgs;

bool Server::readUserData(User &user, bool allowSideEffects) {
  auto xr = XmlReader::FromFile(_userFilesPath + user.name() + ".usr");
  if (!xr) return false;

  auto timeSinceThisDataWasWritten = ms_t{0};
  {
    auto elem = xr.findChild("general");

    auto timeWritten = time_t{0};
    xr.findAttr(elem, "timeThisWasWritten", timeWritten);
    timeSinceThisDataWasWritten =
        static_cast<ms_t>((time(nullptr) - timeWritten) * 1000);

    auto secondsPlayed = 0;
    if (xr.findAttr(elem, "secondsPlayed", secondsPlayed))
      user.secondsPlayedBeforeThisSession(secondsPlayed);

    auto realWorldLocation = ""s;
    if (xr.findAttr(elem, "realWorldLocation", realWorldLocation))
      user.setRealWorldLocation(realWorldLocation);

    auto classID = ClassType::ID{};
    if (!xr.findAttr(elem, "class", classID)) return false;
    auto it = _classes.find(classID);
    if (it == _classes.end()) {
      _debug << Color::CHAT_ERROR << "Invalid class (" << classID
             << ") specified; creating new character." << Log::endl;
    }
    user.setClass(it->second);

    auto level = Level{0};
    if (xr.findAttr(elem, "level", level)) user.level(level);

    auto xp = XP{0};
    if (xr.findAttr(elem, "xp", xp)) user.xp(xp);

    auto n = 0;

    if (!xr.findAttr(elem, "isInTutorial", n) || n != 1)
      user.markTutorialAsCompleted();

    if (xr.findAttr(elem, "health", n)) user.health(n);
    if (xr.findAttr(elem, "energy", n)) user.energy(n);

    if (allowSideEffects) {
      if (xr.findAttr(elem, "isKing", n) && n == 1) makePlayerAKing(user);

      if (xr.findAttr(elem, "isDriving", n) && n == 1) {
        auto pVehicle = _entities.findVehicleDrivenBy(user);
        if (pVehicle) {
          user.driving(pVehicle->serial());
        }
      }
    }
  }

  auto elem = xr.findChild("location");
  auto location = MapPoint{};
  if (elem == nullptr || !xr.findAttr(elem, "x", location.x) ||
      !xr.findAttr(elem, "y", location.y)) {
    _debug("Invalid user data (location)", Color::CHAT_ERROR);
    return false;
  }

  elem = xr.findChild("respawnPoint");
  auto respawnPoint = MapPoint{};
  if (elem && xr.findAttr(elem, "x", respawnPoint.x) &&
      xr.findAttr(elem, "y", respawnPoint.y))
    user.respawnPoint(respawnPoint);

  user.exploration.readFrom(xr);

  if (allowSideEffects) {
    bool s = false;
    if (isLocationValid(location, user))
      user.location(location, /* firstInsertion */ true);
    else {
      _debug << Color::CHAT_ERROR << "Player " << user.name()
             << " was respawned due to an invalid or occupied location."
             << Log::endl;
      user.moveToSpawnPoint(/* firstInsertion */ true);
    }
  } else {
    user.changeDummyLocation(location);
  }

  elem = xr.findChild("inventory");
  for (auto slotElem : xr.getChildren("slot", elem)) {
    int slot;
    std::string id;
    int qty;
    // Default value to support transition of old data
    Hitpoints health = Item::MAX_HEALTH;

    if (!xr.findAttr(slotElem, "slot", slot)) continue;
    if (!xr.findAttr(slotElem, "id", id)) continue;
    if (!xr.findAttr(slotElem, "quantity", qty)) continue;
    xr.findAttr(slotElem, "health", health);

    std::set<ServerItem>::const_iterator it = _items.find(id);
    if (it == _items.end()) {
      _debug("Invalid user data (inventory item).  Removing item.",
             Color::CHAT_ERROR);
      continue;
    }
    user.inventory(slot).first = ServerItem::Instance::LoadFromFile(
        &*it, ServerItem::Instance::ReportingInfo::UserInventory(&user, slot),
        health);
    user.inventory(slot).second = qty;
  }

  elem = xr.findChild("gear");
  for (auto slotElem : xr.getChildren("slot", elem)) {
    int slot;
    std::string id;
    int qty;
    // Default value to support transition of old data
    Hitpoints health = Item::MAX_HEALTH;

    if (!xr.findAttr(slotElem, "slot", slot)) continue;
    if (!xr.findAttr(slotElem, "id", id)) continue;
    if (!xr.findAttr(slotElem, "quantity", qty)) continue;
    xr.findAttr(slotElem, "health", health);

    std::set<ServerItem>::const_iterator it = _items.find(id);
    if (it == _items.end()) {
      _debug("Invalid user data (gear item).  Removing item.",
             Color::CHAT_ERROR);
      continue;
    }
    user.gear(slot).first = ServerItem::Instance::LoadFromFile(
        &*it, ServerItem::Instance::ReportingInfo::UserGear(&user, slot),
        health);
    user.gear(slot).second = qty;
  }

  if (allowSideEffects) {
    elem = xr.findChild("buffs");
    for (auto buffElem : xr.getChildren("buff", elem)) {
      auto id = ""s;
      if (!xr.findAttr(buffElem, "type", id)) continue;
      auto buffTypeIt = _buffTypes.find(id);
      if (buffTypeIt == _buffTypes.end()) continue;

      auto timeRemaining = ms_t{};
      if (!xr.findAttr(buffElem, "timeRemaining", timeRemaining)) continue;
      user.loadBuff(buffTypeIt->second, timeRemaining);
    }
    for (auto buffElem : xr.getChildren("debuff", elem)) {
      auto id = ""s;
      if (!xr.findAttr(buffElem, "type", id)) continue;
      auto buffTypeIt = _buffTypes.find(id);
      if (buffTypeIt == _buffTypes.end()) continue;

      auto timeRemaining = ms_t{};
      if (!xr.findAttr(buffElem, "timeRemaining", timeRemaining)) continue;
      user.loadDebuff(buffTypeIt->second, timeRemaining);
    }
  }

  elem = xr.findChild("knownRecipes");
  for (auto slotElem : xr.getChildren("recipe", elem)) {
    std::string id;
    if (xr.findAttr(slotElem, "id", id)) user.addRecipe(id, false);
  }

  elem = xr.findChild("knownConstructions");
  for (auto slotElem : xr.getChildren("construction", elem)) {
    std::string id;
    if (xr.findAttr(slotElem, "id", id)) user.addConstruction(id, false);
  }

  elem = xr.findChild("talents");
  for (auto talentElem : xr.getChildren("talent", elem)) {
    auto name = ""s;
    if (!xr.findAttr(talentElem, "name", name)) continue;
    auto &userClass = user.getClass();
    auto talent = userClass.type().findTalent(name);
    if (!talent) continue;

    auto rank = 0;
    xr.findAttr(talentElem, "rank", rank);

    userClass.loadTalentRank(*talent, rank);
  }

  elem = xr.findChild("otherSpells");
  for (auto slotElem : xr.getChildren("spell", elem)) {
    std::string id;
    if (xr.findAttr(slotElem, "id", id)) user.getClass().markSpellAsKnown(id);
  }

  elem = xr.findChild("spellCooldowns");
  for (auto slotElem : xr.getChildren("cooldown", elem)) {
    std::string id;
    ms_t remaining;
    if (!xr.findAttr(slotElem, "id", id)) continue;
    if (!xr.findAttr(slotElem, "remaining", remaining)) continue;

    if (timeSinceThisDataWasWritten > remaining) continue;
    remaining -= timeSinceThisDataWasWritten;
    user.loadSpellCooldown(id, remaining);
  }

  elem = xr.findChild("quests");
  for (auto questElem : xr.getChildren("completed", elem)) {
    auto questID = ""s;
    if (!xr.findAttr(questElem, "quest", questID)) continue;
    user.markQuestAsCompleted(questID);
  }
  for (auto questElem : xr.getChildren("inProgress", elem)) {
    auto questID = ""s;
    if (!xr.findAttr(questElem, "quest", questID)) continue;
    auto timeRemaining = ms_t{0};  // Default: no time limit
    xr.findAttr(questElem, "timeRemaining", timeRemaining);
    user.markQuestAsStarted(questID, timeRemaining);

    for (auto progress : xr.getChildren("progress", questElem)) {
      auto typeAsString = ""s;
      if (!xr.findAttr(progress, "type", typeAsString)) continue;
      auto type = Quest::Objective::typeFromString(typeAsString);
      auto id = ""s;
      if (!xr.findAttr(progress, "id", id)) continue;
      auto qty = 0;
      if (!xr.findAttr(progress, "qty", qty)) continue;
      user.initQuestProgress(questID, type, id, qty);
    }
  }

  elem = xr.findChild("hotbar");
  for (auto button : xr.getChildren("button", elem)) {
    auto slot = 0;
    auto category = 0;
    auto id = ""s;
    if (!xr.findAttr(button, "slot", slot)) continue;
    if (!xr.findAttr(button, "category", category)) continue;
    if (!xr.findAttr(button, "id", id)) continue;
    user.setHotbarAction(slot, category, id);
  }

  return true;
}

void Server::writeUserData(const User &user) const {
  ++_threadsOpen;

  XmlWriter xw(_userFilesPath + user.name() + ".usr");

  auto e = xw.addChild("general");
  xw.setAttr(e, "passwordHash", user.pwHash());
  xw.setAttr(e, "timeThisWasWritten", time(nullptr));
  xw.setAttr(e, "secondsPlayed", user.secondsPlayed());
  xw.setAttr(e, "realWorldLocation", user.realWorldLocation());
  xw.setAttr(e, "class", user.getClass().type().id());
  if (_kings.isPlayerAKing(user.name())) xw.setAttr(e, "isKing", 1);
  if (user.isInTutorial()) xw.setAttr(e, "isInTutorial", 1);
  xw.setAttr(e, "level", user.level());
  xw.setAttr(e, "xp", user.xp());
  if (user.isDriving()) xw.setAttr(e, "isDriving", 1);

  if (user.health() < user.stats().maxHealth)
    xw.setAttr(e, "health", user.health());
  if (user.energy() < user.stats().maxEnergy)
    xw.setAttr(e, "energy", user.energy());

  e = xw.addChild("location");
  xw.setAttr(e, "x", user.location().x);
  xw.setAttr(e, "y", user.location().y);

  e = xw.addChild("respawnPoint");
  xw.setAttr(e, "x", user.respawnPoint().x);
  xw.setAttr(e, "y", user.respawnPoint().y);

  e = xw.addChild("inventory");
  for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
    const auto &slot = user.inventory(i);
    if (slot.first.hasItem()) {
      auto slotElement = xw.addChild("slot", e);
      xw.setAttr(slotElement, "slot", i);
      xw.setAttr(slotElement, "id", slot.first.type()->id());
      xw.setAttr(slotElement, "health", slot.first.health());
      xw.setAttr(slotElement, "quantity", slot.second);
    }
  }

  e = xw.addChild("gear");
  for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
    const auto &slot = user.gear(i);
    if (slot.first.hasItem()) {
      auto slotElement = xw.addChild("slot", e);
      xw.setAttr(slotElement, "slot", i);
      xw.setAttr(slotElement, "id", slot.first.type()->id());
      xw.setAttr(slotElement, "health", slot.first.health());
      xw.setAttr(slotElement, "quantity", slot.second);
    }
  }

  e = xw.addChild("buffs");
  for (const auto &buff : user.buffs()) {
    auto buffElement = xw.addChild("buff", e);
    xw.setAttr(buffElement, "type", buff.type());
    xw.setAttr(buffElement, "timeRemaining", buff.timeRemaining());
  }
  for (const auto &debuff : user.debuffs()) {
    auto buffElement = xw.addChild("buff", e);
    xw.setAttr(buffElement, "type", debuff.type());
    xw.setAttr(buffElement, "timeRemaining", debuff.timeRemaining());
  }

  e = xw.addChild("knownRecipes");
  for (const std::string &id : user.knownRecipes()) {
    auto slotElement = xw.addChild("recipe", e);
    xw.setAttr(slotElement, "id", id);
  }

  e = xw.addChild("knownConstructions");
  for (const std::string &id : user.knownConstructions()) {
    auto slotElement = xw.addChild("construction", e);
    xw.setAttr(slotElement, "id", id);
  }

  e = xw.addChild("talents");
  for (auto pair : user.getClass().talentRanks()) {
    if (pair.second == 0) continue;
    auto talentElem = xw.addChild("talent", e);
    xw.setAttr(talentElem, "name", pair.first->name());
    xw.setAttr(talentElem, "rank", pair.second);
  }

  e = xw.addChild("otherSpells");
  for (auto spellID : user.getClass().otherKnownSpells()) {
    auto spellElem = xw.addChild("spell", e);
    xw.setAttr(spellElem, "id", spellID);
  }

  e = xw.addChild("spellCooldowns");
  for (const auto &pair : user.spellCooldowns()) {
    if (pair.second == 0) continue;
    auto cooldownElem = xw.addChild("cooldown", e);
    xw.setAttr(cooldownElem, "id", pair.first);
    xw.setAttr(cooldownElem, "remaining", pair.second);
  }

  e = xw.addChild("quests");
  for (const auto &completedQuestID : user.questsCompleted()) {
    auto questElem = xw.addChild("completed", e);
    xw.setAttr(questElem, "quest", completedQuestID);
  }
  for (const auto &pair : user.questsInProgress()) {
    const auto &questID = pair.first;
    auto questElem = xw.addChild("inProgress", e);
    xw.setAttr(questElem, "quest", questID);
    if (pair.second > 0) xw.setAttr(questElem, "timeRemaining", pair.second);
    auto quest = findQuest(questID);
    for (const auto &objective : quest->objectives) {
      auto progress = user.questProgress(questID, objective.type, objective.id);
      if (progress == 0) continue;
      auto progressElem = xw.addChild("progress", questElem);
      xw.setAttr(progressElem, "type", objective.typeAsString());
      xw.setAttr(progressElem, "id", objective.id);
      xw.setAttr(progressElem, "qty", progress);
    }
  }

  e = xw.addChild("hotbar");
  for (auto i = 0; i != user.hotbar().size(); ++i) {
    const auto &action = user.hotbar()[i];
    if (!action) continue;
    auto actionElem = xw.addChild("button", e);
    xw.setAttr(actionElem, "slot", i);
    xw.setAttr(actionElem, "category", action.category);
    xw.setAttr(actionElem, "id", action.id);
  }

  user.exploration.writeTo(xw);

  xw.publish();
  --_threadsOpen;
}

void Server::loadEntitiesFromFile(const std::string &path,
                                  bool shouldBeExcludedFromPersistentState) {
  // If static, mark them as such.  They will be excluded from being saved to
  // file.

  auto xr = XmlReader::FromFile(path);

  for (auto elem : xr.getChildren("object")) {
    std::string s;
    if (!xr.findAttr(elem, "id", s)) {
      _debug("Skipping importing object with no type.", Color::CHAT_ERROR);
      continue;
    }

    MapPoint p;
    if (!xr.findAttr(elem, "x", p.x) || !xr.findAttr(elem, "y", p.y)) {
      _debug("Skipping importing object with invalid/no location",
             Color::CHAT_ERROR);
      continue;
    }

    const ObjectType *type = findObjectTypeByID(s);
    if (type == nullptr) {
      _debug << Color::CHAT_ERROR
             << "Skipping importing object with unknown type \"" << s << "\"."
             << Log::endl;
      continue;
    }

    auto owner = Permissions::Owner{};
    auto ownerElem = xr.findChild("owner", elem);
    if (ownerElem) {
      std::string type, name;
      xr.findAttr(ownerElem, "type", type);
      xr.findAttr(ownerElem, "name", name);
      if (type == "player")
        owner.type = Permissions::Owner::PLAYER;
      else if (type == "city")
        owner.type = Permissions::Owner::CITY;
      else if (type == "noAccess")
        owner.type = Permissions::Owner::NO_ACCESS;
      else
        _debug << Color::CHAT_ERROR << "Skipping bad object owner type \""
               << type << "\"." << Log::endl;
      owner.name = name;
    }

    Object &obj = addObject(type, p, owner);

    if (shouldBeExcludedFromPersistentState) obj.excludeFromPersistentState();

    size_t n;
    ItemSet gatherContents;
    for (auto content : xr.getChildren("gatherable", elem)) {
      if (!xr.findAttr(content, "id", s)) continue;
      n = 1;
      xr.findAttr(content, "quantity", n);
      auto it = _items.find(s);
      if (it == _items.end()) continue;
      gatherContents.set(&*it, n);
    }
    obj.gatherable.setContents(gatherContents);

    size_t q;
    // Default value to support transition of old data
    Hitpoints health = Item::MAX_HEALTH;
    for (auto inventory : xr.getChildren("inventory", elem)) {
      assert(obj.hasContainer());
      if (!xr.findAttr(inventory, "item", s)) continue;
      if (!xr.findAttr(inventory, "slot", n)) continue;
      q = 1;
      xr.findAttr(inventory, "qty", q);
      xr.findAttr(inventory, "health", health);
      if (obj.objType().container().slots() <= n) {
        _debug << Color::CHAT_ERROR
               << "Skipping object with invalid inventory slot." << Log::endl;
        continue;
      }
      auto &invSlot = obj.container().at(n);
      invSlot.first = ServerItem::Instance::LoadFromFile(
          &*_items.find(s),
          ServerItem::Instance::ReportingInfo::InObjectContainer(), health);
      invSlot.second = q;
    }

    for (auto merchant : xr.getChildren("merchant", elem)) {
      size_t slot;
      if (!xr.findAttr(merchant, "slot", slot)) continue;
      if (slot >= obj.objType().merchantSlots()) continue;
      std::string wareName, priceName;
      if (!xr.findAttr(merchant, "wareItem", wareName) ||
          !xr.findAttr(merchant, "priceItem", priceName))
        continue;
      auto wareIt = _items.find(wareName);
      if (wareIt == _items.end()) continue;
      auto priceIt = _items.find(priceName);
      if (priceIt == _items.end()) continue;
      size_t wareQty = 1, priceQty = 1;
      xr.findAttr(merchant, "wareQty", wareQty);
      xr.findAttr(merchant, "priceQty", priceQty);
      obj.merchantSlot(slot) =
          MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);
    }

    obj.clearMaterialsRequired();
    for (auto material : xr.getChildren("material", elem)) {
      if (!xr.findAttr(material, "id", s)) continue;
      if (!xr.findAttr(material, "qty", n)) continue;
      auto it = _items.find(s);
      if (it == _items.end()) continue;
      obj.remainingMaterials().set(&*it, n);
    }

    if (xr.findAttr(elem, "health", health)) obj.health(health);

    auto corpseTime = ms_t{};
    if (xr.findAttr(elem, "corpseTime", corpseTime)) obj.corpseTime(corpseTime);

    auto transformTimer = ms_t{};
    if (xr.findAttr(elem, "transformTime", transformTimer))
      obj.transformation.transformTimer(transformTimer);

    auto vehicle = xr.findChild("vehicle", elem);
    if (vehicle) {
      auto driver = ""s;
      if (xr.findAttr(vehicle, "driver", driver) && !driver.empty()) {
        auto objAsVehicle = dynamic_cast<Vehicle *>(&obj);
        if (objAsVehicle) {
          objAsVehicle->driver(driver);
        }
      }
    }
  }

  for (auto elem : xr.getChildren("npc")) {
    std::string s;
    if (!xr.findAttr(elem, "id", s)) {
      _debug("Skipping importing NPC with no type.", Color::CHAT_ERROR);
      continue;
    }

    MapPoint p;
    if (!xr.findAttr(elem, "x", p.x) || !xr.findAttr(elem, "y", p.y)) {
      _debug("Skipping importing object with invalid/no location",
             Color::CHAT_ERROR);
      continue;
    }

    const NPCType *type = dynamic_cast<const NPCType *>(findObjectTypeByID(s));
    if (type == nullptr) {
      _debug << Color::CHAT_ERROR
             << "Skipping importing NPC with unknown type \"" << s << "\"."
             << Log::endl;
      continue;
    }

    NPC &npc = addNPC(type, p);

    if (shouldBeExcludedFromPersistentState) npc.excludeFromPersistentState();

    auto health = Hitpoints{};
    if (xr.findAttr(elem, "health", health)) npc.health(health);

    auto corpseTime = ms_t{};
    if (xr.findAttr(elem, "corpseTime", corpseTime)) npc.corpseTime(corpseTime);

    auto ownerElem = xr.findChild("owner", elem);
    if (ownerElem) {
      std::string type, name;
      xr.findAttr(ownerElem, "type", type);
      xr.findAttr(ownerElem, "name", name);
      if (type == "player")
        npc.permissions.setPlayerOwner(name);
      else if (type == "city")
        npc.permissions.setCityOwner(name);
      else
        _debug << Color::CHAT_ERROR << "Skipping bad NPC owner type \"" << type
               << "\"." << Log::endl;
    }

    auto transformTimer = ms_t{};
    if (xr.findAttr(elem, "transformTime", transformTimer))
      npc.transformation.transformTimer(transformTimer);

    auto orderString = ""s;
    if (xr.findAttr(elem, "order", orderString))
      npc.order(orderString == "stay" ? NPC::STAY : NPC::FOLLOW);
  }

  for (auto elem : xr.getChildren("droppedItem")) {
    std::string typeID;
    xr.findAttr(elem, "type", typeID);
    const auto *itemType = findItem(typeID);
    if (!itemType) {
      _debug("Skipping importing item with invalid type "s + typeID,
             Color::CHAT_ERROR);
      continue;
    }

    size_t quantity = 1;
    xr.findAttr(elem, "qty", quantity);

    auto loc = MapPoint{};
    xr.findAttr(elem, "x", loc.x);
    xr.findAttr(elem, "y", loc.y);

    addEntity(new DroppedItem(*itemType, quantity, loc));
  }
}

void Server::loadWorldState(const std::string &path, bool shouldKeepOldData) {
  auto xr = XmlReader::FromFile("");

  std::ifstream fs;
  // Detect/load state
  do {
    bool loadExistingData = !cmdLineArgs.contains("new");

    if (loadExistingData) _cities.readFromXMLFile("World/cities.world");

    // Entities
    loadEntitiesFromFile(path + "/staticObjects.xml", true);
    if (loadExistingData) loadEntitiesFromFile("World/entities.world", false);

    if (!loadExistingData) break;

    _wars.readFromXMLFile("World/wars.world");

    return;
  } while (false);

  // If execution reaches here, fresh objects will be generated instead of old
  // ones loaded.

  _debug("Generating new objects.");
  _dataLoaded = true;
}

void Object::writeToXML(XmlWriter &xw) const {
  if (spawner() != nullptr) return;  // Spawned objects are not persistent.

  auto e = xw.addChild("object");

  xw.setAttr(e, "id", type()->id());

  for (auto &content : gatherable.contents()) {
    auto contentE = xw.addChild("gatherable", e);
    xw.setAttr(contentE, "id", content.first->id());
    xw.setAttr(contentE, "quantity", content.second);
  }

  if (permissions.hasOwner()) {
    const auto &owner = permissions.owner();
    auto ownerElem = xw.addChild("owner", e);
    xw.setAttr(ownerElem, "type", owner.typeString());
    xw.setAttr(ownerElem, "name", owner.name);
  }

  auto loc = xw.addChild("location", e);
  xw.setAttr(loc, "x", location().x);
  xw.setAttr(loc, "y", location().y);

  if (health() < stats().maxHealth) xw.setAttr(e, "health", health());

  if (isDead() && corpseTime() > 0) xw.setAttr(e, "corpseTime", corpseTime());

  if (transformation.transformTimer() > 0)
    xw.setAttr(e, "transformTime", transformation.transformTimer());

  if (hasContainer()) {
    for (size_t i = 0; i != objType().container().slots(); ++i) {
      if (container().at(i).second == 0) continue;
      auto invSlotE = xw.addChild("inventory", e);
      xw.setAttr(invSlotE, "slot", i);
      xw.setAttr(invSlotE, "item", container().at(i).first.type()->id());
      xw.setAttr(invSlotE, "qty", container().at(i).second);
      xw.setAttr(invSlotE, "health", container().at(i).first.health());
    }
  }

  const auto mSlots = merchantSlots();
  for (size_t i = 0; i != mSlots.size(); ++i) {
    if (!mSlots[i]) continue;
    auto mSlotE = xw.addChild("merchant", e);
    xw.setAttr(mSlotE, "slot", i);
    xw.setAttr(mSlotE, "wareItem", mSlots[i].wareItem->id());
    xw.setAttr(mSlotE, "wareQty", mSlots[i].wareQty);
    xw.setAttr(mSlotE, "priceItem", mSlots[i].priceItem->id());
    xw.setAttr(mSlotE, "priceQty", mSlots[i].priceQty);
  }

  for (const auto &pair : remainingMaterials()) {
    auto matE = xw.addChild("material", e);
    xw.setAttr(matE, "id", pair.first->id());
    xw.setAttr(matE, "qty", pair.second);
  }

  auto asVehicle = dynamic_cast<const Vehicle *>(this);
  if (asVehicle && !asVehicle->driver().empty()) {
    auto vehicleE = xw.addChild("vehicle", e);
    xw.setAttr(vehicleE, "driver", asVehicle->driver());
  }
}

void NPC::writeToXML(XmlWriter &xw) const {
  auto e = xw.addChild("npc");

  xw.setAttr(e, "id", type()->id());

  auto loc = xw.addChild("location", e);
  xw.setAttr(loc, "x", location().x);
  xw.setAttr(loc, "y", location().y);

  xw.setAttr(e, "health", health());

  auto constRef = const_cast<const NPC &>(*this);
  if (constRef.permissions.hasOwner()) {
    const auto &owner = constRef.permissions.owner();
    auto ownerElem = xw.addChild("owner", e);
    xw.setAttr(ownerElem, "type", owner.typeString());
    xw.setAttr(ownerElem, "name", owner.name);
  }

  if (transformation.transformTimer() > 0)
    xw.setAttr(e, "transformTime", transformation.transformTimer());

  auto orderString = _order == FOLLOW ? "follow" : "stay";
  xw.setAttr(e, "order", orderString);

  if (isDead() && corpseTime() > 0) xw.setAttr(e, "corpseTime", corpseTime());
}

void DroppedItem::writeToXML(XmlWriter &xw) const {
  auto e = xw.addChild("droppedItem");

  xw.setAttr(e, "type", _itemType.id());
  if (_quantity > 1) xw.setAttr(e, "qty", _quantity);
  xw.setAttr(e, "x", location().x);
  xw.setAttr(e, "y", location().y);
}

void Server::saveData(const Entities &entities, const Wars &wars,
                      const Cities &cities) {
  // Entities
#ifndef SINGLE_THREAD
  static std::mutex entitiesFileMutex;
  entitiesFileMutex.lock();
#endif
  XmlWriter xw("World/entities.world");

  for (const Entity *entity : entities) {
    if (entity->excludedFromPersistentState()) continue;
    entity->writeToXML(xw);
  }

  xw.publish();
#ifndef SINGLE_THREAD
  entitiesFileMutex.unlock();
#endif

  // Wars
#ifndef SINGLE_THREAD
  static std::mutex warsFileMutex;
  warsFileMutex.lock();
#endif
  wars.writeToXMLFile("World/wars.world");
#ifndef SINGLE_THREAD
  warsFileMutex.unlock();
#endif

  // Cities
#ifndef SINGLE_THREAD
  static std::mutex citiesFileMutex;
  citiesFileMutex.lock();
#endif
  cities.writeToXMLFile("World/cities.world");
#ifndef SINGLE_THREAD
  citiesFileMutex.unlock();
#endif
}
