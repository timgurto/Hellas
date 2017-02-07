#include "Client.h"
#include "ClientNPCType.h"
#include "ClientVehicleType.h"
#include "ParticleProfile.h"
#include "ClientVehicleType.h"
#include "../XmlReader.h"

void Client::loadData(const std::string &path){
    _terrain.clear();
    _items.clear();
    _recipes.clear();
    _objectTypes.clear();
    _objects.clear();

    // Load terrain
    XmlReader xr(path + "/terrain.xml");
    if (xr)
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
            _terrain[index] = Terrain(fileName, frames, frameTime);
        }

    // Particles
    if (xr.newFile(path + "/particles.xml"))
        for (auto elem : xr.getChildren("particleProfile")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) // No ID: skip
                continue;
            ParticleProfile *profile = new ParticleProfile(s);
            double mean, sd;
            if (xr.findAttr(elem, "particlesPerSecond", mean)) profile->particlesPerSecond(mean);
            if (xr.findAttr(elem, "gravityModifier", mean)) profile->gravityuModifer(mean);
            if (xr.findNormVarChild("particlesPerHit", elem, mean, sd)) profile->particlesPerHit(mean, sd);
            if (xr.findNormVarChild("distance", elem, mean, sd)) profile->distance(mean, sd);
            if (xr.findNormVarChild("altitude", elem, mean, sd)) profile->altitude(mean, sd);
            if (xr.findNormVarChild("velocity", elem, mean, sd)) profile->velocity(mean, sd);
            if (xr.findNormVarChild("fallSpeed", elem, mean, sd)) profile->fallSpeed(mean, sd);
            if (xr.findNormVarChild("lifespan", elem, mean, sd)) profile->lifespan(mean, sd);

            for (auto variety : xr.getChildren("variety", elem)){
                if (!xr.findAttr(variety, "imageFile", s))
                    continue; // No image file specified; skip
                Rect rect;
                xr.findRectChild("drawRect", variety, rect);
                profile->addVariety(s, rect);
            }

            _particleProfiles.insert(profile);
        }

    // Object types
    if (xr.newFile(path + "/objectTypes.xml"))
        for (auto elem : xr.getChildren("objectType")) {
            std::string s; int n;
            if (!xr.findAttr(elem, "id", s))
                continue;
            ClientObjectType *cot;
            if (xr.findAttr(elem, "isVehicle", n) == 1)
                cot = new ClientVehicleType(s);
            else
                cot = new ClientObjectType(s);
            xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
            cot->image(std::string("Images/Objects/") + s + ".png");
            if (xr.findAttr(elem, "name", s)) cot->name(s);
            Rect drawRect(0, 0, cot->width(), cot->height());
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

            if (xr.findAttr(elem, "merchantSlots", n)) cot->merchantSlots(n);

            if (xr.findAttr(elem, "isFlat", n) && n != 0) cot->isFlat(true);
            if (xr.findAttr(elem, "gatherSound", s))
                cot->gatherSound(std::string("Sounds/") + s + ".wav");
            if (xr.findAttr(elem, "gatherParticles", s)) cot->gatherParticles(findParticleProfile(s));
            Rect r;
            if (xr.findRectChild("collisionRect", elem, r)) cot->collisionRect(r);

            if (cot->classTag() == 'v'){
                auto driver = xr.findChild("driver", elem);
                if (driver != nullptr){
                    ClientVehicleType &vt = dynamic_cast<ClientVehicleType &>(*cot);
                    vt.drawDriver(true);
                    Point offset;
                    xr.findAttr(driver, "x", offset.x);
                    xr.findAttr(driver, "y", offset.y);
                    vt.driverOffset(offset);
                }
            }

            _objectTypes.insert(cot);
        }

    // Items
    if (xr.newFile(path + "/items.xml"))
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

            auto offset = xr.findChild("offset", elem);
            if (offset != nullptr){
                Point drawLoc;
                xr.findAttr(offset, "x", drawLoc.x);
                xr.findAttr(offset, "y", drawLoc.y);
                item.drawLoc(drawLoc);
            }

            size_t gearSlot = GEAR_SLOTS; // Default; won't match any slot.
            xr.findAttr(elem, "gearSlot", gearSlot); item.gearSlot(gearSlot);

            if (xr.findAttr(elem, "constructs", s)){
                // Create dummy ObjectType if necessary
                auto pair = _objectTypes.insert(new ClientObjectType(s));
                item.constructsObject(*pair.first);
            }
        
            std::pair<std::set<ClientItem>::iterator, bool> ret = _items.insert(item);
            if (!ret.second) {
                ClientItem &itemInPlace = const_cast<ClientItem &>(*ret.first);
                itemInPlace = item;
            }
        }

    // Recipes
    if (xr.newFile(path + "/recipes.xml"))
        for (auto elem : xr.getChildren("recipe")) {
            std::string id, name;
            if (!xr.findAttr(elem, "id", id))
                continue; // ID is mandatory.
            Recipe recipe(id);

            std::string s;
            if (!xr.findAttr(elem, "product", s))
                continue; // product is mandatory.
            auto it = _items.find(s);
            if (it == _items.end()) {
                _debug << Color::FAILURE << "Skipping recipe with invalid product " << s << Log::endl;
                continue;
            }
            recipe.product(&*it);

            for (auto child : xr.getChildren("material", elem)) {
                int matQty = 1;
                xr.findAttr(child, "quantity", matQty);
                if (xr.findAttr(child, "id", s)) {
                    auto it = _items.find(ClientItem(s));
                    if (it == _items.end()) {
                        _debug << Color::FAILURE << "Skipping invalid recipe material " << s << Log::endl;
                        continue;
                    }
                    recipe.addMaterial(&*it, matQty);
                }
            }

            for (auto child : xr.getChildren("tool", elem)) {
                if (xr.findAttr(child, "class", s)) {
                    recipe.addTool(s);
                }
            }
        
            _recipes.insert(recipe);
        }

    // NPC types
    if (xr.newFile(path + "/npcTypes.xml"))
        for (auto elem : xr.getChildren("npcType")) {
            std::string s;
            if (!xr.findAttr(elem, "id", s)) // No ID: skip
                continue;
            int n;
            if (!xr.findAttr(elem, "maxHealth", n)) // No max health: skip
                continue;
            ClientNPCType *nt = new ClientNPCType(s, n);
            xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
            nt->image(std::string("Images/NPCs/") + s + ".png");
            nt->corpseImage(std::string("Images/NPCs/") + s + "-corpse.png");
            if (xr.findAttr(elem, "name", s)) nt->name(s);
            Rect drawRect(0, 0, nt->width(), nt->height());
            bool
                xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
                ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
            if (xSet || ySet)
                nt->drawRect(drawRect);
            Rect r;
            if (xr.findRectChild("collisionRect", elem, r)) nt->collisionRect(r);
            auto pair = _objectTypes.insert(nt);
            if (!pair.second) {
                // A ClientObjectType is being pointed to by items; they need to point to this instead.
                const ClientObjectType *dummy = *pair.first;
                for (const ClientItem &item : _items)
                    if (item.constructsObject() == dummy){
                        ClientItem &nonConstItem = const_cast<ClientItem &>(item);
                        nonConstItem.constructsObject(nt);
                    }
                _objectTypes.erase(dummy);
                delete dummy;
                _objectTypes.insert(nt);
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

    _dataLoaded = true;
}
