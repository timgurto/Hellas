#pragma once

#include <string>

class Client;
class XmlReader;

class CDataLoader {
public:
    using Directory = std::string;
    static CDataLoader FromPath(Client &client, const Directory &path = "Data");

    void load(bool keepOldData = false);

    void loadTerrain(XmlReader &reader);
    void loadParticles(XmlReader &reader);
    void loadSounds(XmlReader &reader);
    void loadProjectiles(XmlReader &reader);
    void loadSpells(XmlReader &reader);
    void loadBuffs(XmlReader &reader);
    void loadObjectTypes(XmlReader &reader);
    void loadItems(XmlReader &reader);
    void loadClasses(XmlReader &reader);
    void loadRecipes(XmlReader &reader);
    void loadNPCTypes(XmlReader &reader);
    void loadMap(XmlReader &reader);

private:
    CDataLoader(Client &client);

    Client &_client;
    Directory _path;

    using FilesList = std::set<std::string>;
    FilesList findDataFiles() const;
};
