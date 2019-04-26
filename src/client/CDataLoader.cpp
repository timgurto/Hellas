#include "CDataLoader.h"

#include <set>

#include "../Podes.h"
#include "../XmlReader.h"
#include "ClassInfo.h"
#include "Client.h"
#include "ClientBuff.h"
#include "ClientNPCType.h"
#include "ClientVehicleType.h"
#include "ParticleProfile.h"
#include "SoundProfile.h"

CDataLoader::CDataLoader(Client &client) : _client(client) {}

CDataLoader CDataLoader::FromPath(Client &client, const Directory &path) {
  auto loader = CDataLoader(client);
  loader._path = path;
  return loader;
}

CDataLoader CDataLoader::FromString(Client &client, const XML &data) {
  auto loader = CDataLoader(client);
  loader._data = data;
  return loader;
}

void CDataLoader::load(bool keepOldData) {
  if (!keepOldData) {
    _client._terrain.clear();
    _client._particleProfiles.clear();
    _client._soundProfiles.clear();
    _client._projectileTypes.clear();
    _client._objects.clear();
    _client._items.clear();
    _client._classes.clear();
    _client._recipes.clear();
  }

  _client.drawLoadingScreen("Loading data", 0.6);

  auto usingPath = !_path.empty();
  if (usingPath) {
    _files = findDataFiles();

    loadFromAllFiles(&CDataLoader::loadTerrain);
    loadFromAllFiles(&CDataLoader::loadParticles);
    loadFromAllFiles(&CDataLoader::loadSounds);
    loadFromAllFiles(&CDataLoader::loadProjectiles);
    loadFromAllFiles(&CDataLoader::loadSpells);
    loadFromAllFiles(&CDataLoader::loadBuffs);

    for (const auto &file : _files) {
      auto xr = XmlReader::FromFile(file);
      _client._tagNames.readFromXML(xr);
    }

    loadFromAllFiles(&CDataLoader::loadObjectTypes);
    loadFromAllFiles(&CDataLoader::loadItems);
    loadFromAllFiles(&CDataLoader::loadClasses);
    loadFromAllFiles(&CDataLoader::loadRecipes);
    loadFromAllFiles(&CDataLoader::loadNPCTypes);
    loadFromAllFiles(&CDataLoader::loadQuests);

    _client.drawLoadingScreen("Loading map", 0.65);
    auto reader = XmlReader::FromFile(_path + "/map.xml");
    loadMap(reader);
  } else {
    loadTerrain(XmlReader::FromString(_data));
    loadParticles(XmlReader::FromString(_data));
    loadSounds(XmlReader::FromString(_data));
    loadProjectiles(XmlReader::FromString(_data));
    loadSpells(XmlReader::FromString(_data));
    loadBuffs(XmlReader::FromString(_data));
    _client._tagNames.readFromXML(XmlReader::FromString(_data));
    loadObjectTypes(XmlReader::FromString(_data));
    loadItems(XmlReader::FromString(_data));
    loadClasses(XmlReader::FromString(_data));
    loadRecipes(XmlReader::FromString(_data));
    loadNPCTypes(XmlReader::FromString(_data));
    loadQuests(XmlReader::FromString(_data));
  }

  _client._dataLoaded = true;
}

void CDataLoader::loadFromAllFiles(LoadFunction load) {
  for (const auto &file : _files) {
    auto xr = XmlReader::FromFile(file);
    (this->*load)(xr);
  }
}

CDataLoader::FilesList CDataLoader::findDataFiles() const {
  auto list = FilesList{};

  WIN32_FIND_DATA fd;
  auto path = std::string{_path.begin(), _path.end()} + "/";
  std::replace(path.begin(), path.end(), '/', '\\');
  std::string filter = path + "*.xml";
  path.c_str();
  HANDLE hFind = FindFirstFile(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName == "map.xml"s) continue;
      auto file = path + fd.cFileName;
      list.insert(file);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
  }

  return list;
}

void CDataLoader::loadTerrain(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("terrain")) {
    char index;
    if (!xr.findAttr(elem, "index", index)) continue;
    std::string fileName;
    if (!xr.findAttr(elem, "id", fileName)) continue;
    xr.findAttr(elem, "imageFile", fileName);  // Supercedes id if present.
    int isTraversable = 1;
    xr.findAttr(elem, "isTraversable", isTraversable);
    int frames = 1, frameTime = 0;
    xr.findAttr(elem, "frames", frames);
    xr.findAttr(elem, "frameTime", frameTime);
    _client._terrain[index] = ClientTerrain(fileName, frames, frameTime);
  }
}

void CDataLoader::loadParticles(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("particleProfile")) {
    std::string s;
    if (!xr.findAttr(elem, "id", s))  // No ID: skip
      continue;
    ParticleProfile *profile = new ParticleProfile(s);
    double mean, sd;
    if (xr.findAttr(elem, "particlesPerSecond", mean))
      profile->particlesPerSecond(mean);
    if (xr.findAttr(elem, "gravityModifier", mean))
      profile->gravityModifer(mean);
    if (xr.findNormVarChild("particlesPerHit", elem, mean, sd))
      profile->particlesPerHit(mean, sd);
    if (xr.findNormVarChild("distance", elem, mean, sd))
      profile->distance(mean, sd);
    if (xr.findNormVarChild("altitude", elem, mean, sd))
      profile->altitude(mean, sd);
    if (xr.findNormVarChild("velocity", elem, mean, sd))
      profile->velocity(mean, sd);
    if (xr.findNormVarChild("fallSpeed", elem, mean, sd))
      profile->fallSpeed(mean, sd);
    if (xr.findNormVarChild("lifespan", elem, mean, sd))
      profile->lifespan(mean, sd);
    int n;
    if (xr.findAttr(elem, "noZDimension", n) && n != 0) profile->noZDimension();
    if (xr.findAttr(elem, "canBeUnderground", n) && n != 0)
      profile->canBeUnderground();
    if (xr.findAttr(elem, "alpha", n) && n != 0xff) profile->alpha(n);

    auto dirE = xr.findChild("direction", elem);
    if (dirE) {
      MapPoint direction;
      xr.findAttr(dirE, "x", direction.x);
      xr.findAttr(dirE, "y", direction.y);
      profile->direction(direction);
    }

    for (auto variety : xr.getChildren("variety", elem)) {
      if (!xr.findAttr(variety, "imageFile", s))
        continue;  // No image file specified; skip
      size_t count = 1;
      xr.findAttr(variety, "count", count);
      auto drawRect = ScreenRect{};
      if (xr.findRectChild("drawRect", variety, drawRect))
        profile->addVariety(s, count, drawRect);
      else
        profile->addVariety(s, count);
    }

    _client._particleProfiles.insert(profile);
  }
}

void CDataLoader::loadSounds(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("soundProfile")) {
    std::string id;
    if (!xr.findAttr(elem, "id", id))  // No ID: skip
      continue;
    auto resultPair = _client._soundProfiles.insert(SoundProfile(id));
    SoundProfile &sp = const_cast<SoundProfile &>(*resultPair.first);
    if (id == "avatar")
      _client._avatarSounds = &sp;
    else if (id == "general")
      _client._generalSounds = &sp;

    ms_t period;
    if (xr.findAttr(elem, "period", period)) sp.period(period);

    for (auto sound : xr.getChildren("sound", elem)) {
      std::string type, file;
      if (!xr.findAttr(sound, "type", type) ||
          !xr.findAttr(sound, "file", file))
        continue;
      sp.add(type, file);
    }
  }
}

void CDataLoader::loadProjectiles(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("projectile")) {
    auto id = ""s;
    if (!xr.findAttr(elem, "id", id)) continue;

    auto drawRect = ScreenRect{};
    if (!xr.findRectChild("drawRect", elem, drawRect)) continue;

    Projectile::Type *projectile = new Projectile::Type(id, drawRect);

    xr.findAttr(elem, "speed", projectile->speed);
    xr.findAttr(elem, "particlesAtEnd", projectile->particlesAtEnd);

    auto tailElem = xr.findChild("tail", elem);
    if (tailElem) {
      auto imageFile = ""s;
      xr.findAttr(tailElem, "image", imageFile);

      auto drawRect = ScreenRect{};
      xr.findAttr(tailElem, "x", drawRect.x);
      xr.findAttr(tailElem, "y", drawRect.y);
      xr.findAttr(tailElem, "w", drawRect.w);
      xr.findAttr(tailElem, "h", drawRect.h);

      auto length = 1;
      xr.findAttr(tailElem, "length", length);

      auto separation = 1;
      xr.findAttr(tailElem, "separation", separation);

      auto tailParticles = ""s;
      xr.findAttr(tailElem, "particles", tailParticles);

      projectile->tail(imageFile, drawRect, length, separation, tailParticles);
    }

    auto sounds = ""s;
    if (xr.findAttr(elem, "sounds", sounds)) projectile->sounds(sounds);

    _client._projectileTypes.insert(projectile);
  }
}

void CDataLoader::loadSpells(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("spell")) {
    std::string id;
    if (!xr.findAttr(elem, "id", id)) continue;  // ID is mandatory.
    auto newSpell = new ClientSpell(id);
    _client._spells[id] = newSpell;

    auto icon = ""s;
    if (xr.findAttr(elem, "icon", icon)) newSpell->icon(_client._icons[icon]);

    auto name = ""s;
    if (xr.findAttr(elem, "name", name)) newSpell->name(name);

    auto cost = Energy{};
    if (xr.findAttr(elem, "cost", cost)) newSpell->cost(cost);

    auto school = SpellSchool{};
    if (xr.findAttr(elem, "school", school)) newSpell->school(school);

    auto range = Podes{0};
    if (xr.findAttr(elem, "range", range))
      newSpell->range(range);
    else if (xr.findAttr(elem, "radius", range))
      newSpell->radius(range);

    auto cooldown = 0;
    if (xr.findAttr(elem, "cooldown", cooldown)) newSpell->cooldown(cooldown);

    auto customDescription = ""s;
    if (xr.findAttr(elem, "customDescription", customDescription))
      newSpell->customDescription(customDescription);

    auto functionElem = xr.findChild("function", elem);
    if (functionElem) {
      auto functionName = ""s;
      if (xr.findAttr(functionElem, "name", functionName))
        newSpell->effectName(functionName);

      auto effectArgs = ClientSpell::Args{};
      xr.findAttr(functionElem, "i1", effectArgs.i1);
      xr.findAttr(functionElem, "s1", effectArgs.s1);
      xr.findAttr(functionElem, "d1", effectArgs.d1);

      newSpell->effectArgs(effectArgs);
    }

    auto aesthetics = xr.findChild("aesthetics", elem);
    if (aesthetics) {
      auto profileName = ""s;
      if (xr.findAttr(aesthetics, "projectile", profileName)) {
        auto dummy = Projectile::Type{profileName, {}};
        auto it = _client._projectileTypes.find(&dummy);
        if (it != _client._projectileTypes.end()) newSpell->projectile(*it);
      }
      if (xr.findAttr(aesthetics, "sounds", profileName)) {
        auto profile = _client.findSoundProfile(profileName);
        if (profile != nullptr) newSpell->sounds(profile);
      }
      if (xr.findAttr(aesthetics, "impactParticles", profileName)) {
        auto profile = _client.findParticleProfile(profileName);
        if (profile != nullptr) newSpell->impactParticles(profile);
      }
    }

    // Contains many assumptions about allowed/forbidden combinations.
    auto validTargets = xr.findChild("targets", elem);
    assert(validTargets);
    if (validTargets) {
      auto val = 0;
      auto self = xr.findAttr(validTargets, "self", val) && val != 0;
      auto friendly = xr.findAttr(validTargets, "friendly", val) && val != 0;
      auto enemy = xr.findAttr(validTargets, "enemy", val) && val != 0;

      if (friendly) {
        assert(self);
        if (enemy)
          newSpell->targetType(ClientSpell::TargetType::ALL);
        else
          newSpell->targetType(ClientSpell::TargetType::FRIENDLY);
      } else if (enemy) {
        assert(!self);
        newSpell->targetType(ClientSpell::TargetType::ENEMY);
      } else {
        assert(self);
        newSpell->targetType(ClientSpell::TargetType::SELF);
      }
    }
  }
}

void CDataLoader::loadBuffs(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("buff")) {
    auto id = ClientBuffType::ID{};
    if (!xr.findAttr(elem, "id", id)) continue;  // ID is mandatory.

    auto iconFile = id;
    xr.findAttr(elem, "icon", iconFile);
    auto newBuff = ClientBuffType{iconFile};

    newBuff.id(id);

    auto name = ClientBuffType::Name{};
    if (xr.findAttr(elem, "name", name)) newBuff.name(name);

    int n;
    if (xr.findAttr(elem, "duration", n)) newBuff.duration(n);

    if (xr.findAttr(elem, "cancelsOnOOE", n) && n == 1) newBuff.cancelOnOOE();

    auto particleProfileName = ""s;
    if (xr.findAttr(elem, "particles", particleProfileName))
      newBuff.particles(particleProfileName);

    auto effectElem = xr.findChild("effect", elem);
    if (effectElem) {
      auto filename = ""s;
      xr.findAttr(effectElem, "image", filename);
      filename = "Images/Effects/"s + filename + ".png";
      auto offset = ScreenPoint{};
      xr.findAttr(effectElem, "xOffset", offset.x);
      xr.findAttr(effectElem, "yOffset", offset.y);

      newBuff.effect({filename, Color::MAGENTA}, offset);
    }

    auto functionElem = xr.findChild("function", elem);
    if (functionElem) {
      auto effectName = ""s;
      if (xr.findAttr(functionElem, "name", effectName))
        newBuff.effectName(effectName);

      auto effectArgs = ClientSpell::Args{};
      xr.findAttr(functionElem, "i1", effectArgs.i1);
      xr.findAttr(functionElem, "s1", effectArgs.s1);
      xr.findAttr(functionElem, "d1", effectArgs.d1);

      newBuff.effectArgs(effectArgs);

      if (xr.findAttr(functionElem, "tickTime", n)) newBuff.tickTime(n);
      if (xr.findAttr(functionElem, "onHit", n) && n != 0)
        newBuff.hasHitEffect();
    }

    auto stats = StatsMod{};
    auto itemHasStats = xr.findStatsChild("stats", elem, stats);
    if (itemHasStats) newBuff.stats(stats);

    _client._buffTypes[id] = newBuff;
  }
}

void CDataLoader::loadObjectTypes(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("objectType")) {
    std::string id;
    if (!xr.findAttr(elem, "id", id)) continue;
    int n;
    ClientObjectType *cot;
    if (xr.findAttr(elem, "isVehicle", n) == 1)
      cot = new ClientVehicleType(id);
    else
      cot = new ClientObjectType(id);

    auto imageFile = id;
    xr.findAttr(elem, "imageFile", imageFile);
    cot->imageFile(imageFile);
    cot->setImage(std::string("Images/Objects/") + imageFile + ".png");
    cot->imageSet(std::string("Images/Objects/") + imageFile + ".png");
    cot->corpseImage(std::string("Images/Objects/") + imageFile +
                     "-corpse.png");
    auto name = id;
    xr.findAttr(elem, "name", name);
    cot->name(name);
    ScreenRect drawRect(0, 0, cot->width(), cot->height());
    bool xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
         ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
    if (xSet || ySet) cot->drawRect(drawRect);
    if (xr.getChildren("yield", elem).size() > 0) cot->canGather(true);
    auto s = ""s;
    if (xr.findAttr(elem, "deconstructs", s)) cot->canDeconstruct(true);

    auto container = xr.findChild("container", elem);
    if (container != nullptr) {
      if (xr.findAttr(container, "slots", n)) cot->containerSlots(n);
    }

    for (auto objTag : xr.getChildren("tag", elem))
      if (xr.findAttr(objTag, "name", s)) cot->addTag(s);

    if (xr.findAttr(elem, "merchantSlots", n)) cot->merchantSlots(n);
    if (xr.findAttr(elem, "isFlat", n) && n != 0) cot->isFlat(true);
    if (xr.findAttr(elem, "isDecoration", n) && n != 0) cot->isDecoration(true);
    if (xr.findAttr(elem, "sounds", s)) cot->sounds(s);
    if (xr.findAttr(elem, "gatherParticles", s))
      cot->gatherParticles(_client.findParticleProfile(s));
    if (xr.findAttr(elem, "damageParticles", s))
      cot->damageParticles(_client.findParticleProfile(s));
    if (xr.findAttr(elem, "gatherReq", s)) cot->gatherReq(s);
    if (xr.findAttr(elem, "constructionReq", s)) cot->constructionReq(s);
    if (xr.findAttr(elem, "constructionText", s)) cot->constructionText(s);
    MapRect r;
    if (xr.findRectChild("collisionRect", elem, r)) cot->collisionRect(r);

    if (xr.findAttr(elem, "playerUnique", s)) {
      cot->makePlayerUnique();
      cot->addTag(s + " (1 per player)");
    }

    auto action = xr.findChild("action", elem);
    if (action != nullptr) {
      auto pAction = new ClientObjectAction;

      xr.findAttr(action, "label", pAction->label);
      xr.findAttr(action, "tooltip", pAction->tooltip);
      xr.findAttr(action, "textInput", pAction->textInput);

      auto costID = ""s;
      if (xr.findAttr(action, "cost", costID)) {
        pAction->cost = &_client._items[costID];
      }

      cot->action(pAction);
    }

    bool canConstruct = false;
    for (auto objMat : xr.getChildren("material", elem)) {
      if (!xr.findAttr(objMat, "id", s)) continue;
      ClientItem &item = _client._items[s];
      n = 1;
      xr.findAttr(objMat, "quantity", n);
      cot->addMaterial(&item, n);
      canConstruct = true;
    }
    if (xr.findAttr(elem, "isUnbuildable", n) == 1) canConstruct = false;
    if (canConstruct) {
      bool hasLocks = xr.findChild("unlockedBy", elem) != nullptr;
      if (!hasLocks) _client._knownConstructions.insert(id);
    }

    if (cot->classTag() == 'v') {
      auto driver = xr.findChild("driver", elem);
      if (driver != nullptr) {
        ClientVehicleType &vt = dynamic_cast<ClientVehicleType &>(*cot);
        vt.drawDriver(true);
        ScreenPoint offset;
        xr.findAttr(driver, "x", offset.x);
        xr.findAttr(driver, "y", offset.y);
        vt.driverOffset(offset);
      }
    }

    auto transform = xr.findChild("transform", elem);
    if (transform) {
      if (xr.findAttr(transform, "time", n)) cot->transformTime(n);
      for (auto progress : xr.getChildren("progress", transform)) {
        if (xr.findAttr(progress, "image", s)) cot->addTransformImage(s);
      }
    }

    // Strength
    auto strength = xr.findChild("strength", elem);
    if (strength) {
      if (xr.findAttr(strength, "item", s) &&
          xr.findAttr(strength, "quantity", n)) {
        ClientItem &item = _client._items[s];
        cot->strength(&item, n);
      } else
        _client.showErrorMessage(
            "Transformation specified without target id; skipping.",
            Color::CHAT_ERROR);
    }

    for (auto particles : xr.getChildren("particles", elem)) {
      auto profileName = ""s;
      if (!xr.findAttr(particles, "profile", profileName)) continue;
      MapPoint offset{};
      xr.findAttr(particles, "x", offset.x);
      xr.findAttr(particles, "y", offset.y);
      cot->addParticles(profileName, offset);
    }

    _client._objectTypes.insert(cot);
  }
}

void CDataLoader::loadItems(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("item")) {
    std::string id, name;
    if (!xr.findAttr(elem, "id", id)) continue;  // ID is mandatory.
    if (!xr.findAttr(elem, "name", name)) name = id;
    ClientItem item(id, name);
    std::string s;
    for (auto child : xr.getChildren("tag", elem))
      if (xr.findAttr(child, "name", s)) item.addTag(s);

    if (xr.findAttr(elem, "iconFile", s))
      item.icon(s);
    else
      item.icon(id);
    if (xr.findAttr(elem, "gearFile", s))
      item.gearImage(s);
    else
      item.gearImage(id);

    if (xr.findAttr(elem, "sounds", s)) item.sounds(s);

    Hitpoints strength;
    if (xr.findAttr(elem, "strength", strength)) item.strength(strength);

    auto offset = xr.findChild("offset", elem);
    if (offset != nullptr) {
      ScreenPoint drawLoc;
      xr.findAttr(offset, "x", drawLoc.x);
      xr.findAttr(offset, "y", drawLoc.y);
      item.drawLoc(drawLoc);
    }

    auto stats = StatsMod{};
    auto itemHasStats = xr.findStatsChild("stats", elem, stats);
    if (itemHasStats) item.stats(stats);

    auto weaponElem = xr.findChild("weapon", elem);
    if (weaponElem != nullptr) {
      auto damage = Hitpoints{0};
      xr.findAttr(weaponElem, "damage", damage);

      auto speedInS = 0.0;
      xr.findAttr(weaponElem, "speed", speedInS);

      auto school = ""s;
      xr.findAttr(weaponElem, "school", school);

      item.makeWeapon(damage, speedInS, {school});

      auto range = Podes{};
      if (xr.findAttr(weaponElem, "range", range)) item.weaponRange(range);

      auto ammoType = ""s;
      if (xr.findAttr(weaponElem, "consumes", ammoType))
        item.weaponAmmo({ammoType});

      auto projectile = ""s;
      if (xr.findAttr(weaponElem, "projectile", projectile))
        item.projectile(_client.findProjectileType(projectile));
    }

    size_t gearSlot = _client.GEAR_SLOTS;  // Default; won't match any slot.
    xr.findAttr(elem, "gearSlot", gearSlot);
    item.gearSlot(gearSlot);

    if (xr.findAttr(elem, "constructs", s)) {
      // Create dummy ObjectType if necessary
      auto pair = _client._objectTypes.insert(new ClientObjectType(s));
      item.constructsObject(*pair.first);
    }

    if (xr.findAttr(elem, "castsSpellOnUse", s)) item.castsSpellOnUse(s);

    for (auto particles : xr.getChildren("particles", elem)) {
      auto profileName = ""s;
      if (!xr.findAttr(particles, "profile", profileName)) continue;
      MapPoint offset{};
      xr.findAttr(particles, "x", offset.x);
      xr.findAttr(particles, "y", offset.y);
      item.addParticles(profileName, offset);
    }

    _client._items[id] = item;
  }
}

void CDataLoader::loadClasses(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("class")) {
    auto className = ClassInfo::Name{};
    if (!xr.findAttr(elem, "name", className)) continue;  // Name is mandatory

    auto newClass = ClassInfo{className};

    auto description = ""s;
    if (xr.findAttr(elem, "description", description))
      newClass.description(description);

    for (auto tree : xr.getChildren("tree", elem)) {
      auto treeName = ClassInfo::Name{};
      if (!xr.findAttr(tree, "name", treeName)) continue;  // Name is mandatory
      newClass.ensureTreeExists(treeName);

      auto currentTier = 0;
      for (auto tier : xr.getChildren("tier", tree)) {
        auto cost = xr.findChild("cost", tier);
        auto costTag = ""s;
        auto costQty = 0;
        if (cost) {
          xr.findAttr(cost, "tag", costTag);
          xr.findAttr(cost, "quantity", costQty);
        }
        auto req = xr.findChild("requires", tier);
        auto reqPointsInTree = 0;
        if (req) {
          xr.findAttr(req, "pointsInTree", reqPointsInTree);
        }

        for (auto talent : xr.getChildren("talent", tier)) {
          auto t = ClientTalent{};

          auto typeName = ""s;
          if (!xr.findAttr(talent, "type", typeName)) continue;

          xr.findAttr(talent, "name", t.name);
          xr.findAttr(talent, "flavourText", t.flavourText);

          t.costTag = costTag;
          t.costQuantity = costQty;
          t.reqPointsInTree = reqPointsInTree;
          t.tree = treeName;

          if (typeName == "spell") {
            t.type = ClientTalent::SPELL;

            auto spellID = ""s;
            if (!xr.findAttr(talent, "id", spellID)) continue;
            auto it = _client._spells.find(spellID);
            if (it == _client._spells.end()) continue;
            t.spell = it->second;

            t.icon = t.spell->icon();

            if (t.name.empty()) t.name = t.spell->name();

          } else if (typeName == "stats") {
            if (t.name.empty()) continue;

            t.type = ClientTalent::STATS;

            auto icon = ""s;
            if (xr.findAttr(talent, "icon", icon))
              t.icon = _client._icons[icon];

            auto stats = StatsMod{};
            if (!xr.findStatsChild("stats", talent, stats)) continue;
            t.stats = stats;
          }

          t.generateLearnMessage();
          newClass.addTalentToTree(t, treeName, currentTier);
        }

        ++currentTier;
      }
    }

    _client._classes[className] = std::move(newClass);
  }
}

void CDataLoader::loadRecipes(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("recipe")) {
    std::string id;
    if (!xr.findAttr(elem, "id", id)) continue;  // ID is mandatory.
    Recipe recipe(id);

    std::string s = id;
    xr.findAttr(elem, "product", s);
    auto it = _client._items.find(s);
    if (it == _client._items.end()) {
      _client.showErrorMessage("Skipping recipe with invalid product "s + s,
                               Color::CHAT_ERROR);
      continue;
    }
    const ClientItem *item = &it->second;
    recipe.product(item);

    auto name = item->name();
    xr.findAttr(elem, "name", name);
    recipe.name(name);

    size_t n;
    if (xr.findAttr(elem, "quantity", n)) recipe.quantity(n);

    for (auto child : xr.getChildren("material", elem)) {
      int matQty = 1;
      xr.findAttr(child, "quantity", matQty);
      if (xr.findAttr(child, "id", s)) {
        auto it = _client._items.find(s);
        if (it == _client._items.end()) {
          _client.showErrorMessage("Skipping invalid recipe material "s + s,
                                   Color::CHAT_ERROR);
          continue;
        }
        const ClientItem *material = &it->second;
        recipe.addMaterial(material, matQty);
      }
    }

    for (auto child : xr.getChildren("tool", elem)) {
      if (xr.findAttr(child, "class", s)) {
        recipe.addTool(s);
      }
    }

    if (xr.getChildren("unlockedBy", elem).empty())
      _client._knownRecipes.insert(id);

    _client._recipes.insert(recipe);
  }
}

void CDataLoader::loadNPCTypes(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("npcType")) {
    auto id = ""s;
    if (!xr.findAttr(elem, "id", id))  // No ID: skip
      continue;

    auto maxHealth = 1;
    xr.findAttr(elem, "maxHealth", maxHealth);

    auto humanoid = xr.findChild("humanoid", elem);

    auto imagePath = ""s;
    if (humanoid) {
      auto baseImage = "default"s;
      xr.findAttr(humanoid, "base", baseImage);
      imagePath = "Images/Humans/"s + baseImage;
    } else {
      auto imageFile = id;
      xr.findAttr(elem, "imageFile",
                  imageFile);  // If no explicit imageFile, will still == id
      imagePath = "Images/NPCs/"s + imageFile;
    }

    ClientNPCType *nt = new ClientNPCType(id, imagePath, maxHealth);

    auto s = id;
    xr.findAttr(elem, "name", s);
    nt->name(s);

    // Draw rect
    auto drawRect = humanoid ? Avatar::DRAW_RECT
                             : ScreenRect{0, 0, nt->width(), nt->height()};
    bool xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
         ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
    nt->drawRect(drawRect);

    // Collision rect
    if (humanoid) nt->collisionRect(Avatar::COLLISION_RECT);
    auto r = MapRect{};
    if (xr.findRectChild("collisionRect", elem, r)) nt->collisionRect(r);

    if (xr.findAttr(elem, "sounds", s)) nt->sounds(s);

    if (xr.findAttr(elem, "projectile", s)) {
      auto dummy = Projectile::Type{s, {}};
      auto it = _client._projectileTypes.find(&dummy);
      if (it != _client._projectileTypes.end()) nt->projectile(**it);
    }

    for (auto objTag : xr.getChildren("class", elem))
      if (xr.findAttr(objTag, "name", s)) nt->addTag(s);

    for (auto particles : xr.getChildren("particles", elem)) {
      auto profileName = ""s;
      if (!xr.findAttr(particles, "profile", profileName)) continue;
      MapPoint offset{};
      xr.findAttr(particles, "x", offset.x);
      xr.findAttr(particles, "y", offset.y);
      nt->addParticles(profileName, offset);
    }

    for (auto gearElem : xr.getChildren("gear", humanoid)) {
      auto id = ""s;
      if (!xr.findAttr(gearElem, "id", id)) continue;

      auto it = _client._items.find(id);
      if (it == _client._items.end()) {
        _client.showErrorMessage("Skipping invalid NPC gear "s + id,
                                 Color::CHAT_ERROR);
        continue;
      }

      nt->addGear(it->second);
    }

    auto n = 0;
    if (xr.findAttr(elem, "isCivilian", n) && n != 0)
      nt->makeCivilian();
    else if (xr.findAttr(elem, "isNeutral", n) && n != 0)
      nt->makeNeutral();

    // Insert
    auto pair = _client._objectTypes.insert(nt);
    if (!pair.second) {
      // A ClientObjectType is being pointed to by items; they need to point to
      // this instead.
      const ClientObjectType *dummy = *pair.first;
      for (const auto &pair : _client._items) {
        const ClientItem &item = pair.second;
        if (item.constructsObject() == dummy) {
          ClientItem &nonConstItem = const_cast<ClientItem &>(item);
          nonConstItem.constructsObject(nt);
        }
      }
      _client._objectTypes.erase(dummy);
      delete dummy;
      _client._objectTypes.insert(nt);
    }
  }
}

void CDataLoader::loadQuests(XmlReader &xr) {
  if (!xr) return;

  for (auto elem : xr.getChildren("quest")) {
    CQuest::Info questInfo;

    if (!xr.findAttr(elem, "id", questInfo.id)) continue;  // ID is mandatory.

    if (!xr.findAttr(elem, "startsAt", questInfo.startsAt))
      continue;  // Start node is mandatory

    if (!xr.findAttr(elem, "endsAt", questInfo.endsAt))
      continue;  // End node is mandatory

    xr.findAttr(elem, "name", questInfo.name);
    xr.findAttr(elem, "brief", questInfo.brief);
    xr.findAttr(elem, "debrief", questInfo.debrief);
    xr.findAttr(elem, "helpTopicOnAccept", questInfo.helpTopicOnAccept);
    xr.findAttr(elem, "helpTopicOnComplete", questInfo.helpTopicOnComplete);

    for (auto objectiveElem : xr.getChildren("objective", elem)) {
      auto &client = Client::instance();
      auto objective = CQuest::Info::Objective{};

      auto id = ""s;
      xr.findAttr(objectiveElem, "id", id);

      auto type = ""s;
      xr.findAttr(objectiveElem, "type", type);

      if (type == "kill") {
        auto npcType = client.findNPCType(id);
        objective.text = "Kill "s + (npcType ? npcType->name() : "???"s);

      } else if (type == "construct") {
        auto objType = client.findObjectType(id);
        objective.text = "Construct "s + (objType ? objType->name() : "???"s);

      } else if (type == "fetch") {
        auto &it = client._items.find(id);
        objective.text = it->second.name();
      }

      objective.qty = 1;
      xr.findAttr(objectiveElem, "qty", objective.qty);

      questInfo.objectives.push_back(objective);
    }

    _client._quests[questInfo.id] = {questInfo};
  }
}

void CDataLoader::loadMap(XmlReader &xr) {
  if (!xr) {
    return;
  }

  auto elem = xr.findChild("size");
  if (elem == nullptr || !xr.findAttr(elem, "x", _client._mapX) ||
      !xr.findAttr(elem, "y", _client._mapY)) {
    _client.showErrorMessage("Map size missing or incomplete.",
                             Color::CHAT_ERROR);
    return;
  }
  _client._map = std::vector<std::vector<char> >(_client._mapX);
  for (size_t x = 0; x != _client._mapX; ++x)
    _client._map[x] = std::vector<char>(_client._mapY, 0);
  for (auto row : xr.getChildren("row")) {
    size_t y;
    auto rowNumberSpecified = xr.findAttr(row, "y", y);
    if (!rowNumberSpecified) {
      _client.showErrorMessage("Map row is missing a row number.",
                               Color::CHAT_ERROR);
      return;
    }
    if (y >= _client._mapY) {
      _client.showErrorMessage("Map row number "s + toString(y) +
                                   " exceeds y dimension of " +
                                   toString(_client._mapY),
                               Color::CHAT_ERROR);
      return;
    }
    std::string rowTerrain;
    if (!xr.findAttr(row, "terrain", rowTerrain)) {
      _client.showErrorMessage("Map row is missing terrain information.",
                               Color::CHAT_ERROR);
      return;
    }
    for (size_t x = 0; x != rowTerrain.size(); ++x) {
      if (x > _client._mapX) {
        _client.showErrorMessage("Row length of "s + toString(x) +
                                     " exceeds x dimension of " +
                                     toString(_client._mapX),
                                 Color::CHAT_ERROR);
        return;
      }
      _client._map[x][y] = rowTerrain[x];
    }
  }
  return;
}
