// (C) 2016 Tim Gurto

#include "Client.h"
#include "ClientNPCType.h"
#include "ParticleProfile.h"
#include "../XmlReader.h"

/*
If a child exists with the specified name, attempt to read its 'mean' and 'sd' attributes into
the provided variables.  Use default values of mean=0, sd=1.
mean and sd will be changed if and only if the child is found.
Return value: whether or not the child was found.
*/
static bool findNormVarChild(const std::string &val, TiXmlElement *elem, double &mean, double &sd){
    TiXmlElement *child = XmlReader::findChild(val, elem);
    if (child == nullptr)
        return false;
    if (!XmlReader::findAttr(child, "mean", mean)) mean = 0;
    if (!XmlReader::findAttr(child, "sd", sd)) sd = 1;
    return true;
}

/*
If a child exists with the specified name, attempt to read its 'x', 'y', 'w', and 'h' attributes
into the provided Rect variable.  It will be changed if and only if the child is found.
Any missing attributes will be assumed to be 0.
Return value: whether or not the child was found.
*/
static bool findRectChild(const std::string &val, TiXmlElement *elem, Rect &rect){
    TiXmlElement *child = XmlReader::findChild(val, elem);
    if (child == nullptr)
        return false;
    if (!XmlReader::findAttr(child, "x", rect.x)) rect.x = 0;
    if (!XmlReader::findAttr(child, "y", rect.y)) rect.y = 0;
    if (!XmlReader::findAttr(child, "w", rect.w)) rect.w = 0;
    if (!XmlReader::findAttr(child, "h", rect.h)) rect.h = 0;
    return true;
}

void Client::loadData(const std::string &path){
    _terrain.clear();
    _items.clear();
    _recipes.clear();
    _objectTypes.clear();
    _objects.clear();

    // Load terrain
    XmlReader xr(path + "/terrain.xml");
    for (auto elem : xr.getChildren("terrain")) {
        int index;
        if (!xr.findAttr(elem, "index", index))
            continue;
        std::string fileName;
        if (!xr.findAttr(elem, "imageFile", fileName))
            continue;
        int isTraversable = 1;
        xr.findAttr(elem, "isTraversable", isTraversable);
        if (index >= static_cast<int>(_terrain.size()))
            _terrain.resize(index+1);
        int frames = 1, frameTime = 0;
        xr.findAttr(elem, "frames", frames);
        xr.findAttr(elem, "frameTime", frameTime);
        _terrain[index] = Terrain(fileName, isTraversable != 0, frames, frameTime);
    }

    // Particles
    xr.newFile(path + "/particles.xml");
    for (auto elem : xr.getChildren("particleProfile")) {
        std::string s;
        if (!xr.findAttr(elem, "id", s)) // No ID: skip
            continue;
        ParticleProfile *profile = new ParticleProfile(s);
        double mean, sd;
        if (xr.findAttr(elem, "particlesPerSecond", mean)) profile->particlesPerSecond(mean);
        if (findNormVarChild("particlesPerHit", elem, mean, sd)) profile->particlesPerHit(mean, sd);
        if (findNormVarChild("distance", elem, mean, sd)) profile->distance(mean, sd);
        if (findNormVarChild("altitude", elem, mean, sd)) profile->altitude(mean, sd);
        if (findNormVarChild("velocity", elem, mean, sd)) profile->velocity(mean, sd);
        if (findNormVarChild("fallSpeed", elem, mean, sd)) profile->fallSpeed(mean, sd);

        for (auto variety : xr.getChildren("variety", elem)){
            if (!xr.findAttr(variety, "imageFile", s))
                continue; // No image file specified; skip
            Rect rect;
            findRectChild("drawRect", variety, rect);
            profile->addVariety(s, rect);
        }

        _particleProfiles.insert(profile);
    }

    // Load Items
    xr.newFile(path + "/items.xml");
    for (auto elem : xr.getChildren("item")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
            continue; // ID and name are mandatory.
        ClientItem item(id, name);
        std::string s;
        for (auto child : xr.getChildren("class", elem))
            if (xr.findAttr(child, "name", s)) item.addClass(s);

        if (xr.findAttr(elem, "iconFile", s))
            item.icon(s);
        else
            item.icon(id);

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
    xr.newFile(path + "/recipes.xml");
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
            _debug << Color::MMO_RED << "Skipping recipe with invalid product " << s << Log::endl;
            continue;
        }
        recipe.product(&*it);

        for (auto child : xr.getChildren("material", elem)) {
            int matQty = 1;
            xr.findAttr(child, "quantity", matQty);
            if (xr.findAttr(child, "id", s)) {
                auto it = _items.find(ClientItem(s));
                if (it == _items.end()) {
                    _debug << Color::MMO_RED << "Skipping invalid recipe material " << s << Log::endl;
                    continue;
                }
                recipe.addMaterial(&*it, matQty);
            }
        }

        for (auto child : xr.getChildren("tool", elem)) {
            if (xr.findAttr(child, "name", s)) {
                recipe.addTool(s);
            }
        }
        
        _recipes.insert(recipe);
    }

    // Object types
    xr.newFile(path + "/objectTypes.xml");
    for (auto elem : xr.getChildren("objectType")) {
        std::string s; int n;
        if (!xr.findAttr(elem, "id", s))
            continue;
        ClientObjectType *cot = new ClientObjectType(s);
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
        if (findRectChild("collisionRect", elem, r)) cot->collisionRect(r);
        auto pair = _objectTypes.insert(cot);
        if (!pair.second) {
            ClientObjectType &type = const_cast<ClientObjectType &>(**pair.first);
            type = *cot;
            delete cot;
        }
    }

    // NPC types
    xr.newFile(path + "/npcTypes.xml");
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
        if (findRectChild("collisionRect", elem, r)) nt->collisionRect(r);
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

}
