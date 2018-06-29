#pragma once

#include <string>

class Client;

class DataLoader {
public:
    using Directory = std::string;
    DataLoader(Client &client, const Directory &path = "Data");

    void load(bool keepOldData = false);

    void loadTerrain(const std::string &filename);
    void loadParticles(const std::string &filename);
    void loadSounds(const std::string &filename);
    void loadProjectiles(const std::string &filename);
    void loadSpells(const std::string &filename);
    void loadBuffs(const std::string &filename);
    void loadObjectTypes(const std::string &filename);
    void loadItems(const std::string &filename);
    void loadClasses(const std::string &filename);
    void loadRecipes(const std::string &filename);
    void loadNPCTypes(const std::string &filename);
    void loadMap(const std::string &filename);

private:
    Client &_client;
    Directory _path;

    using FilesList = std::set<std::string>;
    FilesList findDataFiles() const;
};
