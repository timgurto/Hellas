#ifndef SINGLE_THREAD
#include <mutex>
#include <thread>
#endif

#include "NPCType.h"
#include "Server.h"
#include "VehicleType.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"

extern Args cmdLineArgs;

bool Server::readUserData(User &user){
    XmlReader xr((std::string("Users/") + user.name() + ".usr").c_str());
    if (!xr)
        return false;

    auto elem = xr.findChild("general");
    std::string className;
    if (!xr.findAttr(elem, "class", className))
        return false;
    auto it = User::CLASS_CODES.find(className);
    if (it == User::CLASS_CODES.end()){
        _debug << Color::RED << "Invalid class (" << className
               << ") specified; creating new character." << Log::endl;
    }
    user.setClass(it->second);

    elem = xr.findChild("location");
    Point p;
    if (elem == nullptr || !xr.findAttr(elem, "x", p.x) || !xr.findAttr(elem, "y", p.y)) {
            _debug("Invalid user data (location)", Color::RED);
            return false;
    }
    bool randomizedLocation = false;
    while (!isLocationValid(p, User::OBJECT_TYPE)) {
        p = mapRand();
        randomizedLocation = true;
    }
    if (randomizedLocation)
        _debug << Color::YELLOW << "Player " << user.name()
               << " was moved due to an invalid location." << Log::endl;
    user.location(p);

    elem = xr.findChild("inventory");
    for (auto slotElem : xr.getChildren("slot", elem)) {
        int slot; std::string id; int qty;
        if (!xr.findAttr(slotElem, "slot", slot)) continue;
        if (!xr.findAttr(slotElem, "id", id)) continue;
        if (!xr.findAttr(slotElem, "quantity", qty)) continue;

        std::set<ServerItem>::const_iterator it = _items.find(id);
        if (it == _items.end()) {
            _debug("Invalid user data (inventory item).  Removing item.", Color::RED);
            continue;
        }
        user.inventory(slot) =
            std::make_pair<const ServerItem *, size_t>(&*it, static_cast<size_t>(qty));
    }

    elem = xr.findChild("gear");
    for (auto slotElem : xr.getChildren("slot", elem)) {
        int slot; std::string id; int qty;
        if (!xr.findAttr(slotElem, "slot", slot)) continue;
        if (!xr.findAttr(slotElem, "id", id)) continue;
        if (!xr.findAttr(slotElem, "quantity", qty)) continue;

        std::set<ServerItem>::const_iterator it = _items.find(id);
        if (it == _items.end()) {
            _debug("Invalid user data (gear item).  Removing item.", Color::RED);
            continue;
        }
        user.gear(slot) =
            std::make_pair<const ServerItem *, size_t>(&*it, static_cast<size_t>(qty));
    }

    elem = xr.findChild("knownRecipes");
    for (auto slotElem : xr.getChildren("recipe", elem)){
        std::string id;
        if (xr.findAttr(slotElem, "id", id)) user.addRecipe(id);
    }

    elem = xr.findChild("knownConstructions");
    for (auto slotElem : xr.getChildren("construction", elem)){
        std::string id;
        if (xr.findAttr(slotElem, "id", id)) user.addConstruction(id);
    }

    elem = xr.findChild("stats");
    unsigned n;
    if (xr.findAttr(elem, "health", n))
        user.health(n);

    return true;

}

void Server::writeUserData(const User &user) const{
    XmlWriter xw(std::string("Users/") + user.name() + ".usr");

    auto e = xw.addChild("general");
    xw.setAttr(e, "class", User::CLASS_NAMES[user.getClass()]);

    e = xw.addChild("location");
    xw.setAttr(e, "x", user.location().x);
    xw.setAttr(e, "y", user.location().y);

    e = xw.addChild("inventory");
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const std::pair<const ServerItem *, size_t> &slot = user.inventory(i);
        if (slot.first) {
            auto slotElement = xw.addChild("slot", e);
            xw.setAttr(slotElement, "slot", i);
            xw.setAttr(slotElement, "id", slot.first->id());
            xw.setAttr(slotElement, "quantity", slot.second);
        }
    }

    e = xw.addChild("gear");
    for (size_t i = 0; i != User::GEAR_SLOTS; ++i) {
        const std::pair<const ServerItem *, size_t> &slot = user.gear(i);
        if (slot.first != nullptr) {
            auto slotElement = xw.addChild("slot", e);
            xw.setAttr(slotElement, "slot", i);
            xw.setAttr(slotElement, "id", slot.first->id());
            xw.setAttr(slotElement, "quantity", slot.second);
        }
    }

    e = xw.addChild("knownRecipes");
    for (const std::string &id : user.knownRecipes()){
        auto slotElement = xw.addChild("recipe", e);
        xw.setAttr(slotElement, "id", id);
    }

    e = xw.addChild("knownConstructions");
    for (const std::string &id : user.knownConstructions()){
        auto slotElement = xw.addChild("construction", e);
        xw.setAttr(slotElement, "id", id);
    }

    e = xw.addChild("stats");
    xw.setAttr(e, "health", user.health());

    xw.publish();
}

void Server::loadData(const std::string &path){
    _objectTypes.clear();
    _items.clear();
    _recipes.clear();
    _objects.clear();

    // Load terrain lists
    XmlReader xr(path + "/terrain.xml");
    std::map<std::string, char> terrainCodes; // For easier lookup when compiling lists below.
    if (xr){
        for (auto elem : xr.getChildren("terrain")) {
            char index;
            std::string id;
            if (!xr.findAttr(elem, "index", index))
                continue;
            if (!xr.findAttr(elem, "id", id))
                continue;
            terrainCodes[id] = index;
        }

        for (auto elem : xr.getChildren("list")){
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            TerrainList tl;
            for (auto terrain : xr.getChildren("allow", elem)){
                std::string s;
                if (xr.findAttr(terrain, "id", s) && terrainCodes.find(s) != terrainCodes.end())
                    tl.allow(terrainCodes[s]);
            }
            for (auto terrain : xr.getChildren("forbid", elem)){
                std::string s;
                if (xr.findAttr(terrain, "id", s) && terrainCodes.find(s) != terrainCodes.end())
                    tl.forbid(terrainCodes[s]);
            }
            TerrainList::addList(id, tl);
            size_t default = 0;
            if (xr.findAttr(elem, "default", default) && default == 1)
                TerrainList::setDefault(id);
        }
    }

    // Object types
    if (!xr.newFile(path + "/objectTypes.xml"))
        _debug("Failed to load objectTypes.xml", Color::FAILURE);
    else
        for (auto elem : xr.getChildren("objectType")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            ObjectType *ot = nullptr;
            int n;
            if (xr.findAttr(elem, "isVehicle", n) == 1) 
                ot = new VehicleType(id);
            else
                ot = new ObjectType(id);

            std::string s;
            if (xr.findAttr(elem, "gatherTime", n)) ot->gatherTime(n);
            if (xr.findAttr(elem, "constructionTime", n)) ot->constructionTime(n);
            if (xr.findAttr(elem, "gatherReq", s)) ot->gatherReq(s);
            if (xr.findAttr(elem, "deconstructs", s)){
                std::set<ServerItem>::const_iterator itemIt = _items.insert(ServerItem(s)).first;
                ot->deconstructsItem(&*itemIt);
            }
            if (xr.findAttr(elem, "deconstructionTime", n)) ot->deconstructionTime(n);

            // Gathering yields
            for (auto yield : xr.getChildren("yield", elem)) {
                if (!xr.findAttr(yield, "id", s))
                    continue;
                double initMean = 1., initSD = 0, gatherMean = 1, gatherSD = 0;
                xr.findAttr(yield, "initialMean", initMean);
                xr.findAttr(yield, "initialSD", initSD);
                xr.findAttr(yield, "gatherMean", gatherMean);
                xr.findAttr(yield, "gatherSD", gatherSD);
                std::set<ServerItem>::const_iterator itemIt = _items.insert(ServerItem(s)).first;
                ot->addYield(&*itemIt, initMean, initSD, gatherMean, gatherSD);
            }

            // Merchant
            if (xr.findAttr(elem, "merchantSlots", n)) ot->merchantSlots(n);
            if (xr.findAttr(elem, "bottomlessMerchant", n)) ot->bottomlessMerchant(n == 1);

            Rect r;
            if (xr.findRectChild("collisionRect", elem, r))
                ot->collisionRect(r);

            // Tags
            for (auto objTag :xr.getChildren("tag", elem))
                if (xr.findAttr(objTag, "name", s))
                    ot->addTag(s);

            // Construction site
            for (auto objMat : xr.getChildren("material", elem)){
                if (!xr.findAttr(objMat, "id", s))
                    continue;
                auto it = _items.find(s);
                if (it == _items.end()){
                    _debug("Invalid construction material specified; skipping");
                    continue;
                }
                n = 1;
                xr.findAttr(objMat, "quantity", n);
                ot->addMaterial(&*it, n);
            }
            for (auto objUnlock : xr.getChildren("unlockedBy", elem)){
                if (xr.findAttr(objUnlock, "id", s))
                    _constructionLocks[s].insert(id);

            }

            // Container
            auto container = xr.findChild("container", elem);
            if (container != nullptr) {
                if (xr.findAttr(container, "slots", n)) ot->containerSlots(n);
            }

            // Terrain restrictions
            if (xr.findAttr(elem, "allowedTerrain", s))
                ot->allowedTerrain(s);
        
            _objectTypes.insert(ot);
        }

    // NPC types
    if (!xr.newFile(path + "/npcTypes.xml"))
        _debug("Failed to load npcTypes.xml", Color::FAILURE);
    else
        for (auto elem : xr.getChildren("npcType")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id)) // No ID: skip
                continue;
            int n;
            if (!xr.findAttr(elem, "maxHealth", n)) // No health: skip
                continue;
            NPCType *nt = new NPCType(id, n);

            std::string s;
            Rect r;
            if (xr.findRectChild("collisionRect", elem, r))
                nt->collisionRect(r);
            for (auto objTag :xr.getChildren("class", elem))
                if (xr.findAttr(objTag, "name", s))
                    nt->addTag(s);
            if (xr.findAttr(elem, "allowedTerrain", s))
                nt->allowedTerrain(s);

            if (xr.findAttr(elem, "health", n)) nt->maxHealth(n);
            if (xr.findAttr(elem, "attack", n)) nt->attack(n);
            if (xr.findAttr(elem, "attackTime", n)) nt->attackTime(n);

            for (auto loot : xr.getChildren("loot", elem)){
                if (!xr.findAttr(loot, "id", s))
                    continue;

                double mean=1, sd=0;
                xr.findNormVarChild("normal", loot, mean, sd);
                std::set<ServerItem>::const_iterator itemIt = _items.insert(ServerItem(s)).first;
                nt->addLoot(&*itemIt, mean, sd);
            }
        
            _objectTypes.insert(nt);
        }

    // Items
    if (!xr.newFile(path + "/items.xml"))
        _debug("Failed to load items.xml", Color::FAILURE);
    else
        for (auto elem : xr.getChildren("item")) {
            std::string id, name;
            if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
                continue; // ID and name are mandatory.
            ServerItem item(id);

            std::string s; int n;
            if (xr.findAttr(elem, "stackSize", n))
                item.stackSize(n);
            else
                item.stackSize(1);
            if (xr.findAttr(elem, "constructs", s)){
                // Create dummy ObjectType if necessary
                const ObjectType *ot = findObjectTypeByName(s);
                if (ot != nullptr)
                    item.constructsObject(ot);
                else
                    item.constructsObject(*(_objectTypes.insert(new ObjectType(s)).first));
            }

            auto statsElem = xr.findChild("stats", elem);
            if (statsElem != nullptr){
                StatsMod stats;
                xr.findAttr(statsElem, "health", stats.health);
                xr.findAttr(statsElem, "attack", stats.attack);
                xr.findAttr(statsElem, "attackTime", stats.attackTime);
                xr.findAttr(statsElem, "speed", stats.speed);
                item.stats(stats);
            }

            n = User::GEAR_SLOTS; // Default; won't match any slot.
            xr.findAttr(elem, "gearSlot", n); item.gearSlot(n);

            for (auto child : xr.getChildren("tag", elem))
                if (xr.findAttr(child, "name", s)) item.addTag(s);
        
            std::pair<std::set<ServerItem>::iterator, bool> ret = _items.insert(item);
            if (!ret.second) {
                ServerItem &itemInPlace = const_cast<ServerItem &>(*ret.first);
                itemInPlace = item;
            }
        }

    // Recipes
    if (!xr.newFile(path + "/recipes.xml"))
        _debug("Failed to load recipes.xml", Color::FAILURE);
    else
        for (auto elem : xr.getChildren("recipe")) {
            std::string id, name;
            if (!xr.findAttr(elem, "id", id))
                continue; // ID is mandatory.
            Recipe recipe(id);

            std::string s; int n;
            if (!xr.findAttr(elem, "product", s))
                continue; // product is mandatory.
            auto it = _items.find(s);
            if (it == _items.end()) {
                _debug << Color::RED << "Skipping recipe with invalid product " << s << Log::endl;
                continue;
            }
            recipe.product(&*it);

            if (xr.findAttr(elem, "quantity", n)) recipe.quantity(n);
            if (xr.findAttr(elem, "time", n)) recipe.time(n);

            for (auto child : xr.getChildren("material", elem)) {
                int quantity = 1;
                std::string matID;
                // Quantity
                xr.findAttr(child, "quantity", quantity);
                if (xr.findAttr(child, "id", matID)) {
                    auto it = _items.find(ServerItem(matID));
                    if (it == _items.end()) {
                        _debug << Color::RED << "Skipping invalid recipe material " << matID << Log::endl;
                        continue;
                    }
                    recipe.addMaterial(&*it, quantity);

                    // Locks
                    int unlocks = 0;
                    if (xr.findAttr(child, "unlocks", unlocks) && unlocks != 0)
                        _recipeLocks[matID].insert(id);
                }
            }

            for (auto child : xr.getChildren("tool", elem)) {
                if (xr.findAttr(child, "class", s)) {
                    recipe.addTool(s);

                    // Locks
                    int unlocks = 0;
                    if (xr.findAttr(child, "unlocks", unlocks) && unlocks != 0)
                        _recipeToolLocks[s].insert(id);
                }
            }
        
            _recipes.insert(recipe);
        }

    // Remove invalid items referred to by objects/recipes
    for (auto it = _items.begin(); it != _items.end(); ){
        if (!it->valid()){
            auto temp = it;
            ++it;
            _items.erase(temp);
        } else
            ++it;
    }

    // Spawners
    if (!xr.newFile(path + "/spawnPoints.xml"))
        _debug("Failed to load spawnPoints.xml", Color::FAILURE);
    else{
        for (auto elem : xr.getChildren("spawnPoint")) {
            std::string id;
            if (!xr.findAttr(elem, "type", id)) {
                _debug("Skipping importing spawner with no type.", Color::RED);
                continue;
            }

            size_t index;
            if (!xr.findAttr(elem, "index", index)){
                _debug("Skipping importing spawner with no index.", Color::RED);
                continue;
            }
            if (_spawners.find(index) != _spawners.end()){
                _debug << "Skipping importing spawner with duplicate index " << index << Log::endl;
                continue;
            }

            Point p;
            if (!xr.findAttr(elem, "x", p.x) || !xr.findAttr(elem, "y", p.y)) {
                _debug("Skipping importing spawner with invalid/no location", Color::RED);
                continue;
            }

            const ObjectType *type = findObjectTypeByName(id);
            if (type == nullptr){
                _debug << Color::RED << "Skipping importing spawner for unknown objects \"" << id
                        << "\"." << Log::endl;
                continue;
            }

            Spawner s(index, p, type);

            size_t n;
            if (xr.findAttr(elem, "quantity", n)) s.quantity(n);
            if (xr.findAttr(elem, "respawnTime", n)) s.respawnTime(n);
            double d;
            if (xr.findAttr(elem, "radius", d)) s.radius(d);

            char c;
            for (auto terrain : xr.getChildren("allowedTerrain", elem))
                if (xr.findAttr(terrain, "index", c)) s.allowTerrain(c);

            _spawners[index] = s;
        }
    }

    // Map
    bool mapSuccessful = false;
    do {
        xr.newFile("Data/map.xml");
        if (!xr)
            break;
        auto elem = xr.findChild("size");
        if (elem == nullptr || !xr.findAttr(elem, "x", _mapX) || !xr.findAttr(elem, "y", _mapY)) {
            _debug("Map size missing or incomplete.", Color::RED);
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
        _debug("Failed to load map.", Color::RED);

    std::ifstream fs;
    // Detect/load state
    do {

        // Objects
        if (cmdLineArgs.contains("new"))
            xr.newFile("Data/staticObjects.xml");
        else
            xr.newFile("World/Objects.world");
        if (!xr)
            break;
        for (auto elem : xr.getChildren("object")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) {
                _debug("Skipping importing object with no type.", Color::RED);
                continue;
            }

            Point p;
            auto loc = xr.findChild("location", elem);
            if (!xr.findAttr(loc, "x", p.x) || !xr.findAttr(loc, "y", p.y)) {
                _debug("Skipping importing object with invalid/no location", Color::RED);
                continue;
            }

            const ObjectType *type = findObjectTypeByName(s);
            if (type == nullptr){
                _debug << Color::RED << "Skipping importing object with unknown type \"" << s
                       << "\"." << Log::endl;
                continue;
            }

            Object &obj = addObject(type, p, nullptr);

            if (xr.findAttr(elem, "owner", s)) obj.owner(s);

            size_t n;
            if (xr.findAttr(elem, "spawner", n)){
                auto it = _spawners.find(n);
                if (it != _spawners.end())
                    obj.spawner(&it->second);
            }

            ItemSet contents;
            for (auto content : xr.getChildren("gatherable", elem)) {
                if (!xr.findAttr(content, "id", s))
                    continue;
                n = 1;
                xr.findAttr(content, "quantity", n);
                contents.set(&*_items.find(s), n);
            }
            obj.contents(contents);

            size_t q;
            for (auto inventory : xr.getChildren("inventory", elem)) {
                if (!xr.findAttr(inventory, "item", s))
                    continue;
                if (!xr.findAttr(inventory, "slot", n))
                    continue;
                q = 1;
                xr.findAttr(inventory, "qty", q);
                if (obj.container().size() <= n) {
                    _debug << Color::RED << "Skipping object with invalid inventory slot." << Log::endl;
                    continue;
                }
                auto &invSlot = obj.container()[n];
                invSlot.first = &*_items.find(s);
                invSlot.second = q;
            }

            for (auto merchant : xr.getChildren("merchant", elem)) {
                size_t slot;
                if (!xr.findAttr(merchant, "slot", slot))
                    continue;
                std::string wareName, priceName;
                if (!xr.findAttr(merchant, "wareItem", wareName) ||
                    !xr.findAttr(merchant, "priceItem", priceName))
                    continue;
                auto wareIt = _items.find(wareName);
                if (wareIt == _items.end())
                    continue;
                auto priceIt = _items.find(priceName);
                if (priceIt == _items.end())
                    continue;
                size_t wareQty = 1, priceQty = 1;
                xr.findAttr(merchant, "wareQty", wareQty);
                xr.findAttr(merchant, "priceQty", priceQty);
                obj.merchantSlot(slot) = MerchantSlot(&*wareIt, wareQty, &*priceIt, priceQty);
            }
        }

        if (cmdLineArgs.contains("new"))
            break;

        // NPCs
        xr.newFile("World/npcs.world");
        if (!xr)
            break;
        for (auto elem : xr.getChildren("npc")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) {
                _debug("Skipping importing NPC with no type.", Color::RED);
                continue;
            }

            Point p;
            auto loc = xr.findChild("location", elem);
            if (!xr.findAttr(loc, "x", p.x) || !xr.findAttr(loc, "y", p.y)) {
                _debug("Skipping importing object with invalid/no location", Color::RED);
                continue;
            }

            const NPCType *type = dynamic_cast<const NPCType *>(findObjectTypeByName(s));
            if (type == nullptr){
                _debug << Color::RED << "Skipping importing NPC with unknown type \"" << s
                       << "\"." << Log::endl;
                continue;
            }

            NPC &npc= addNPC(type, p);

            size_t n;
            if (xr.findAttr(elem, "spawner", n)){
                auto it = _spawners.find(n);
                if (it != _spawners.end())
                    npc.spawner(&it->second);
            }
        }

        _dataLoaded = true;
        return;
    } while (false);

    // If execution reaches here, a fresh map will be generated.

    // Static objects
    xr.newFile("data/staticObjects.xml");
    if (xr){
        for (auto elem : xr.getChildren("object"))
    }

    _debug("Generating new objects.", Color::YELLOW);
    spawnInitialObjects();
    _dataLoaded = true;
}

void Server::saveData(const objects_t &objects){
    // Objects
#ifndef SINGLE_THREAD
    static std::mutex objectsFileMutex;
    objectsFileMutex.lock();
#endif
    XmlWriter xw("World/objects.world");
    for (const Object *obj : objects) {
        if (obj->classTag() == 'n')
            continue;
        if (obj->type() == nullptr)
            continue;
        auto e = xw.addChild("object");

        xw.setAttr(e, "id", obj->type()->id());

        for (auto &content : obj->contents()) {
            auto contentE = xw.addChild("gatherable", e);
            xw.setAttr(contentE, "id", content.first->id());
            xw.setAttr(contentE, "quantity", content.second);
        }

        if (!obj->owner().empty())
            xw.setAttr(e, "owner", obj->owner());

        if (obj->spawner() != nullptr)
            xw.setAttr(e, "spawner", obj->spawner()->index());

        auto loc = xw.addChild("location", e);
        xw.setAttr(loc, "x", obj->location().x);
        xw.setAttr(loc, "y", obj->location().y);

        const auto container = obj->container();
        for (size_t i = 0; i != container.size(); ++i) {
            if (container[i].second == 0)
                continue;
            auto invSlotE = xw.addChild("inventory", e);
            xw.setAttr(invSlotE, "slot", i);
            xw.setAttr(invSlotE, "item", container[i].first->id());
            xw.setAttr(invSlotE, "qty", container[i].second);
        }

        const auto mSlots = obj->merchantSlots();
        for (size_t i = 0; i != mSlots.size(); ++i){
            if (!mSlots[i])
                continue;
            auto mSlotE = xw.addChild("merchant", e);
            xw.setAttr(mSlotE, "slot", i);
            xw.setAttr(mSlotE, "wareItem", mSlots[i].wareItem->id());
            xw.setAttr(mSlotE, "wareQty", mSlots[i].wareQty);
            xw.setAttr(mSlotE, "priceItem", mSlots[i].priceItem->id());
            xw.setAttr(mSlotE, "priceQty", mSlots[i].priceQty);
        }
    }
    xw.publish();
#ifndef SINGLE_THREAD
    objectsFileMutex.unlock();
#endif

    // NPCs
#ifndef SINGLE_THREAD
    static std::mutex npcsFileMutex;
    npcsFileMutex.lock();
#endif
    xw.newFile("World/npcs.world");
    for (const Object *obj : objects) {
        if (obj->classTag() != 'n')
            continue;
        if (obj->type() == nullptr)
            continue;
        const NPC &npc = *reinterpret_cast<const NPC *>(obj);
        auto e = xw.addChild("npc");

        xw.setAttr(e, "id", npc.type()->id());

        auto loc = xw.addChild("location", e);
        xw.setAttr(loc, "x", npc.location().x);
        xw.setAttr(loc, "y", npc.location().y);

        xw.setAttr(e, "health", npc.health());
    }
    xw.publish();
#ifndef SINGLE_THREAD
    npcsFileMutex.unlock();
#endif
}

