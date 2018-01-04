#include "ClassInfo.h"
#include "Client.h"
#include "ClientBuff.h"
#include "ClientNPCType.h"
#include "ClientVehicleType.h"
#include "ParticleProfile.h"
#include "ClientVehicleType.h"

#include "SoundProfile.h"
#include "../Podes.h"
#include "../XmlReader.h"

void Client::loadData(const std::string &path){
    drawLoadingScreen("Clearing old data", 0.611);
    _items.clear();
    _recipes.clear();
    _objects.clear();

    // Load terrain
    drawLoadingScreen("Loading terrain", 0.622);
    XmlReader xr(path + "/terrain.xml");
    if (xr){
        _terrain.clear();
        for (auto elem : xr.getChildren("terrain")) {
            char index;
            if (!xr.findAttr(elem, "index", index))
                continue;
            std::string fileName;
            if (!xr.findAttr(elem, "id", fileName))
                continue;
            xr.findAttr(elem, "imageFile", fileName); // Supercedes id if present.
            int isTraversable = 1;
            xr.findAttr(elem, "isTraversable", isTraversable);
            int frames = 1, frameTime = 0;
            xr.findAttr(elem, "frames", frames);
            xr.findAttr(elem, "frameTime", frameTime);
            _terrain[index] = ClientTerrain(fileName, frames, frameTime);
        }
    }

    // Particles
    drawLoadingScreen("Loading particles", 0.633);
    if (xr.newFile(path + "/particles.xml")){
        _particleProfiles.clear();
        for (auto elem : xr.getChildren("particleProfile")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) // No ID: skip
                continue;
            ParticleProfile *profile = new ParticleProfile(s);
            double mean, sd;
            if (xr.findAttr(elem, "particlesPerSecond", mean)) profile->particlesPerSecond(mean);
            if (xr.findAttr(elem, "gravityModifier", mean)) profile->gravityModifer(mean);
            if (xr.findNormVarChild("particlesPerHit", elem, mean, sd)) profile->particlesPerHit(mean, sd);
            if (xr.findNormVarChild("distance", elem, mean, sd)) profile->distance(mean, sd);
            if (xr.findNormVarChild("altitude", elem, mean, sd)) profile->altitude(mean, sd);
            if (xr.findNormVarChild("velocity", elem, mean, sd)) profile->velocity(mean, sd);
            if (xr.findNormVarChild("fallSpeed", elem, mean, sd)) profile->fallSpeed(mean, sd);
            if (xr.findNormVarChild("lifespan", elem, mean, sd)) profile->lifespan(mean, sd);
            int n;
            if (xr.findAttr(elem, "noZDimension", n) && n != 0) profile->noZDimension();
            if (xr.findAttr(elem, "alpha", n) && n != 0xff) profile->alpha(n);

            auto dirE = xr.findChild("direction", elem);
            if (dirE){
                MapPoint direction;
                xr.findAttr(dirE, "x", direction.x);
                xr.findAttr(dirE, "y", direction.y);
                profile->direction(direction);
            }

            for (auto variety : xr.getChildren("variety", elem)){
                if (!xr.findAttr(variety, "imageFile", s))
                    continue; // No image file specified; skip
                size_t count = 1;
                xr.findAttr(variety, "count", count);
                auto drawRect = ScreenRect{};
                if (xr.findRectChild("drawRect", variety, drawRect))
                    profile->addVariety(s, count, drawRect);
                else
                    profile->addVariety(s, count);
            }

            _particleProfiles.insert(profile);
        }
    }
    Avatar::_combatantType.damageParticles(findParticleProfile("blood"));

    // Projectiles
    if (xr.newFile(path + "/projectiles.xml")) {
        _projectileTypes.clear();
        for (auto elem : xr.getChildren("projectile")) {

            auto id = ""s;
            if (!xr.findAttr(elem, "id", id))
                continue;

            auto drawRect = ScreenRect{};
            if (!xr.findRectChild("drawRect", elem, drawRect))
                continue;

            Projectile::Type *projectile = new Projectile::Type(id, drawRect);

            xr.findAttr(elem, "speed", projectile->speed);
            xr.findAttr(elem, "particlesAtEnd", projectile->particlesAtEnd);

            _projectileTypes.insert(projectile);
        }
    }

    // Sounds
    drawLoadingScreen("Loading sounds", 0.638);
    if (xr.newFile(path + "/sounds.xml")){
        _soundProfiles.clear();
        for (auto elem : xr.getChildren("soundProfile")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id)) // No ID: skip
                continue;
            auto resultPair = _soundProfiles.insert(SoundProfile(id));
            SoundProfile &sp = const_cast<SoundProfile &>(*resultPair.first);
            if (id == "avatar")
                _avatarSounds = &sp;

            ms_t period;
            if (xr.findAttr(elem, "period", period))
                sp.period(period);

            for (auto sound : xr.getChildren("sound", elem)){
                std::string type, file;
                    if (!xr.findAttr(sound, "type", type) ||
                        !xr.findAttr(sound, "file", file))
                            continue;
                sp.add(type, file);
            }
        }
    }

    // Spells
    if (!xr.newFile(path + "/spells.xml"))
        _debug("Failed to load spells.xml", Color::FAILURE);
    else {
        for (auto elem : xr.getChildren("spell")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue; // ID is mandatory.
            auto newSpell = new ClientSpell(id);
            _spells[id] = newSpell;

            auto name = ""s;
            if (xr.findAttr(elem, "name", name)) newSpell->name(name);

            auto cost = Energy{};
            if (xr.findAttr(elem, "cost", cost)) newSpell->cost(cost);

            auto school = SpellSchool{};
            if (xr.findAttr(elem, "school", school))
                newSpell->school(school);

            auto range = Podes{0};
            if (xr.findAttr(elem, "range", range)) newSpell->range(range);
            else if (xr.findAttr(elem, "radius", range)) newSpell->radius(range);

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
                    auto dummy = Projectile::Type{ profileName, {} };
                    auto it = _projectileTypes.find(&dummy);
                    if (it != _projectileTypes.end())
                        newSpell->projectile(*it);
                }
                if (xr.findAttr(aesthetics, "sounds", profileName)) {
                    auto profile = findSoundProfile(profileName);
                    if (profile != nullptr)
                        newSpell->sounds(profile);
                }
                if (xr.findAttr(aesthetics, "impactParticles", profileName)) {
                    auto profile = findParticleProfile(profileName);
                    if (profile != nullptr)
                        newSpell->impactParticles(profile);
                }
            }
        }
    }

    // Buffs
    if (!xr.newFile(path + "/buffs.xml"))
        _debug("Failed to load buffs.xml", Color::FAILURE);
    else {
        for (auto elem : xr.getChildren("buff")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue; // ID is mandatory.
            auto newBuff = ClientBuffType{ id };

            auto name = ClientBuffType::Name{};
            if (xr.findAttr(elem, "name", name)) newBuff.name(name);

            _buffTypes[id] = newBuff;
        }
    }

    _tagNames.readFromXMLFile(path + "/tags.xml");

    if (xr.newFile(path + "/items.xml")) // Early, because object types may insert new items.
        _items.clear();

    // Object types
    bool objectTypesCleared = false;
    drawLoadingScreen("Loading objects", 0.644);
    if (xr.newFile(path + "/objectTypes.xml")){
        _objectTypes.clear();
        objectTypesCleared = true;
        for (auto elem : xr.getChildren("objectType")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            int n;
            ClientObjectType *cot;
            if (xr.findAttr(elem, "isVehicle", n) == 1)
                cot = new ClientVehicleType(id);
            else
                cot = new ClientObjectType(id);
            xr.findAttr(elem, "imageFile", id); // If no explicit imageFile, s will still == id
            cot->setImage(std::string("Images/Objects/") + id + ".png");
            cot->imageSet(std::string("Images/Objects/") + id + ".png");
            cot->corpseImage(std::string("Images/Objects/") + id + "-corpse.png");
            std::string s;
            if (xr.findAttr(elem, "name", s)) cot->name(s);
            ScreenRect drawRect(0, 0, cot->width(), cot->height());
            bool
                xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
                ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
            if (xSet || ySet)
                cot->drawRect(drawRect);
            if (xr.getChildren("yield", elem).size() > 0) cot->canGather(true);
            if (xr.findAttr(elem, "deconstructs", s)) cot->canDeconstruct(true);

            auto container = xr.findChild("container", elem);
            if (container != nullptr) {
                if (xr.findAttr(container, "slots", n)) cot->containerSlots(n);
            }

            for (auto objTag :xr.getChildren("tag", elem))
                if (xr.findAttr(objTag, "name", s))
                    cot->addTag(s);

            if (xr.findAttr(elem, "merchantSlots", n)) cot->merchantSlots(n);
            if (xr.findAttr(elem, "isFlat", n) && n != 0) cot->isFlat(true);
            if (xr.findAttr(elem, "isDecoration", n) && n != 0) cot->isDecoration(true);
            if (xr.findAttr(elem, "sounds", s)) cot->sounds(s);
            if (xr.findAttr(elem, "gatherParticles", s)) cot->gatherParticles(findParticleProfile(s));
            if (xr.findAttr(elem, "damageParticles", s)) cot->damageParticles(findParticleProfile(s));
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
                cot->action(pAction);
            }

            bool canConstruct = false;
            for (auto objMat : xr.getChildren("material", elem)){
                if (!xr.findAttr(objMat, "id", s))
                    continue;
                ClientItem &item = _items[s];
                n = 1;
                xr.findAttr(objMat, "quantity", n);
                cot->addMaterial(&item, n);
                canConstruct = true;
            }
            if (xr.findAttr(elem, "isUnbuildable", n) == 1)
                canConstruct = false;
            if (canConstruct){
                bool hasLocks = xr.findChild("unlockedBy", elem) != nullptr;
                if (! hasLocks)
                    _knownConstructions.insert(id);
            }

            if (cot->classTag() == 'v'){
                auto driver = xr.findChild("driver", elem);
                if (driver != nullptr){
                    ClientVehicleType &vt = dynamic_cast<ClientVehicleType &>(*cot);
                    vt.drawDriver(true);
                    ScreenPoint offset;
                    xr.findAttr(driver, "x", offset.x);
                    xr.findAttr(driver, "y", offset.y);
                    vt.driverOffset(offset);
                }
            }

            auto transform = xr.findChild("transform", elem);
            if (transform){
                if (xr.findAttr(transform, "time", n))
                    cot->transformTime(n);
                for (auto progress : xr.getChildren("progress", transform)){
                    if (xr.findAttr(progress, "image", s))
                        cot->addTransformImage(s);
                }
            }

            // Strength
            auto strength = xr.findChild("strength", elem);
            if (strength){
                if (xr.findAttr(strength, "item", s) &&
                    xr.findAttr(strength, "quantity", n)){
                        ClientItem &item = _items[s];
                        cot->strength(&item, n);
                } else
                    _debug("Transformation specified without target id; skipping.", Color::FAILURE);
            }

            _objectTypes.insert(cot);
        }
    }

    // Items
    drawLoadingScreen("Loading items", 0.656);
    if (xr.newFile(path + "/items.xml")){
        for (auto elem : xr.getChildren("item")) {
            std::string id, name;
            if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
                continue; // ID and name are mandatory.
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

            if (xr.findAttr(elem, "sounds", s))
                item.sounds(s);

            Hitpoints strength;
            if (xr.findAttr(elem, "strength", strength))
                item.strength(strength);

            auto offset = xr.findChild("offset", elem);
            if (offset != nullptr){
                ScreenPoint drawLoc;
                xr.findAttr(offset, "x", drawLoc.x);
                xr.findAttr(offset, "y", drawLoc.y);
                item.drawLoc(drawLoc);
            }

            auto stats = StatsMod{};
            auto itemHasStats = xr.findStatsChild("stats", elem, stats);
            if (itemHasStats)
                item.stats(stats);

            auto weaponElem = xr.findChild("weapon", elem);
            if (weaponElem != nullptr) {
                auto range = Podes{};
                if (xr.findAttr(weaponElem, "range", range)) item.weaponRange(range);
            }

            size_t gearSlot = GEAR_SLOTS; // Default; won't match any slot.
            xr.findAttr(elem, "gearSlot", gearSlot); item.gearSlot(gearSlot);

            if (xr.findAttr(elem, "constructs", s)){
                // Create dummy ObjectType if necessary
                auto pair = _objectTypes.insert(new ClientObjectType(s));
                item.constructsObject(*pair.first);
            }

            if (xr.findAttr(elem, "castsSpellOnUse", s))
                item.castsSpellOnUse(s);
        
            _items[id] = item;
        }
    }

    // Classes/talents
    if (!xr.newFile(path + "/classes.xml"))
        _debug("Failed to load classes.xml", Color::FAILURE);
    else {
        _classes.clear();
        for (auto elem : xr.getChildren("class")) {

            auto className = ClassInfo::Name{};
            if (!xr.findAttr(elem, "name", className))
                continue; // Name is mandatory

            auto newClass = ClassInfo{ className };

            for (auto tree : xr.getChildren("tree", elem)) {

                auto treeName = ClassInfo::Name{};
                if (!xr.findAttr(tree, "name", treeName))
                    continue; // Name is mandatory
                newClass.ensureTreeExists(treeName);

                auto currentTier = 0;
                for (auto tier : xr.getChildren("tier", tree)) {

                    for (auto talent : xr.getChildren("talent", tier)) {
                        auto t = ClientTalent{};

                        auto typeName = ""s;
                        if (!xr.findAttr(talent, "type", typeName))
                            continue;

                        xr.findAttr(talent, "name", t.name);
                        xr.findAttr(talent, "flavourText", t.flavourText);

                        if (typeName == "spell") {
                            t.type = ClientTalent::SPELL;

                            auto spellID = ""s;
                            if (!xr.findAttr(talent, "id", spellID))
                                continue;
                            auto it = _spells.find(spellID);
                            if (it == _spells.end())
                                continue;
                            t.spell = it->second;

                            t.icon = t.spell->icon();

                            if (t.name.empty())
                                t.name = t.spell->name();

                        } else if (typeName == "stats") {
                            if (t.name.empty())
                                continue;

                            t.type = ClientTalent::STATS;
                            t.icon = { "Images/Talents/"s + t.name + ".png"s };

                            auto stats = StatsMod{};
                            if (!xr.findStatsChild("stats", talent, stats))
                                continue;
                            t.stats = stats;
                        }

                        t.generateLearnMessage();
                        newClass.addTalentToTree(t, treeName, currentTier);
                    }

                    ++currentTier;
                }
            }

            _classes[className] = std::move(newClass);
        }
    }

    // Initialize object-type strengths
    for (auto *objectType : _objectTypes){
        auto nonConstType = const_cast<ClientObjectType *>(objectType);
        nonConstType->calculateAndInitStrength();
    }

    // Recipes
    drawLoadingScreen("Loading recipes", 0.667);
    if (xr.newFile(path + "/recipes.xml")){
        _recipes.clear();
        for (auto elem : xr.getChildren("recipe")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue; // ID is mandatory.
            Recipe recipe(id);

            std::string s = id;
            xr.findAttr(elem, "product", s);
            auto it = _items.find(s);
            if (it == _items.end()) {
                _debug << Color::FAILURE << "Skipping recipe with invalid product " << s << Log::endl;
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
                    auto it = _items.find(s);
                    if (it == _items.end()) {
                        _debug << Color::FAILURE << "Skipping invalid recipe material " << s << Log::endl;
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
                _knownRecipes.insert(id);
        
            _recipes.insert(recipe);
        }
    }

    // NPC types
    drawLoadingScreen("Loading creatures", 0.678);
    if (xr.newFile(path + "/npcTypes.xml")){
        if (!objectTypesCleared)
            _objectTypes.clear();
        for (auto elem : xr.getChildren("npcType")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) // No ID: skip
                continue;
            int n;
            if (!xr.findAttr(elem, "maxHealth", n)) // No max health: skip
                continue;
            ClientNPCType *nt = new ClientNPCType(s, n);
            xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
            nt->setImage(std::string("Images/NPCs/") + s + ".png");
            nt->imageSet(std::string("Images/NPCs/") + s + ".png");
            nt->corpseImage(std::string("Images/NPCs/") + s + "-corpse.png");
            if (xr.findAttr(elem, "name", s)) nt->name(s);
            ScreenRect drawRect(0, 0, nt->width(), nt->height());
            bool
                xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
                ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
            if (xSet || ySet)
                nt->drawRect(drawRect);
            MapRect r;
            if (xr.findRectChild("collisionRect", elem, r)) nt->collisionRect(r);

            if (xr.findAttr(elem, "sounds", s))
                nt->sounds(s);

            auto pair = _objectTypes.insert(nt);
            if (!pair.second) {
                // A ClientObjectType is being pointed to by items; they need to point to this instead.
                const ClientObjectType *dummy = *pair.first;
                for (const auto &pair: _items){
                    const ClientItem &item = pair.second;
                    if (item.constructsObject() == dummy){
                        ClientItem &nonConstItem = const_cast<ClientItem &>(item);
                        nonConstItem.constructsObject(nt);
                    }
                }
                _objectTypes.erase(dummy);
                delete dummy;
                _objectTypes.insert(nt);
            }

            for (auto particles : xr.getChildren("particles", elem)) {
                auto profileName = ""s;
                if (!xr.findAttr(particles, "profile", profileName))
                    continue;
                MapPoint offset{};
                xr.findAttr(particles, "x", offset.x);
                xr.findAttr(particles, "y", offset.y);
                nt->addParticles(profileName, offset);
            }
        }
    }

    // Map
    drawLoadingScreen("Loading map", 0.689);
    bool mapSuccessful = false;
    do {
        xr.newFile(path + "/map.xml");
        if (!xr)
            break;
        auto elem = xr.findChild("size");
        if (elem == nullptr || !xr.findAttr(elem, "x", _mapX) || !xr.findAttr(elem, "y", _mapY)) {
            _debug("Map size missing or incomplete.", Color::FAILURE);
            break;
        }
        _map = std::vector<std::vector<char> >(_mapX);
        for (size_t x = 0; x != _mapX; ++x)
            _map[x] = std::vector<char>(_mapY, 0);
        for (auto row : xr.getChildren("row")) {
            size_t y;
            if (!xr.findAttr(row, "y", y) || y >= _mapY)
                break;
            std::string rowTerrain;
                if (!xr.findAttr(row, "terrain", rowTerrain))
                    break;
            for (size_t x = 0; x != rowTerrain.size(); ++x){
                if (x > _mapX)
                    break;
                _map[x][y] = rowTerrain[x];
            }
        }
        mapSuccessful = true;
    } while (false);
    if (!mapSuccessful)
        _debug("Failed to load map.", Color::FAILURE);


    populateBuildList();

    _dataLoaded = true;
}
