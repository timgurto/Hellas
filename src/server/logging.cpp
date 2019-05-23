#include <fstream>
#include <utility>

#include "../versionUtil.h"
#include "Server.h"

static int toSeconds(_FILETIME windowsTime) {
  ULONGLONG timeLastOnline = (((ULONGLONG)windowsTime.dwHighDateTime) << 32) +
                             windowsTime.dwLowDateTime;
  _FILETIME ftNow;
  _SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ftNow);
  ULONGLONG timeNow =
      (((ULONGLONG)ftNow.dwHighDateTime) << 32) + ftNow.dwLowDateTime;

  auto timeOffline = timeNow - timeLastOnline;

  static const auto SECONDS = ((ULONGLONG)10000000);
  return static_cast<int>(timeOffline / SECONDS);
}

void Server::writeUserToFile(const User &user, std::ofstream &stream) const {
  auto secondsOnlineOrOffline =
      user.hasSocket() ? user.secondsPlayedThisSession() : user.secondsOffline;

  stream << "\n{"
         << "name: \"" << user.name() << "\","
         << "online: " << user.hasSocket() << ","
         << "secondsPlayed: " << user.secondsPlayed() << ","
         << "secondsOnlineOrOffline: " << secondsOnlineOrOffline << ","
         << "class: \"" << user.getClass().type().id() << "\","
         << "level: \"" << user.level() << "\","
         << "xp: \"" << user.xp() << "\","
         << "xpNeeded: \"" << user.XP_PER_LEVEL[user.level()] << "\","
         << "x: \"" << user.location().x << "\","
         << "y: \"" << user.location().y << "\","
         << "city: \"" << _cities.getPlayerCity(user.name()) << "\","
         << "isKing: " << _kings.isPlayerAKing(user.name()) << ","
         << "health: " << user.health() << ","
         << "maxHealth: " << user.stats().maxHealth << ","
         << "energy: " << user.energy() << ","
         << "maxEnergy: " << user.stats().maxEnergy << ","
         << "knownRecipes: " << user.knownRecipes().size() << ","
         << "completedQuests: " << user.questsCompleted().size() << ","
         << "knownConstructions: " << user.knownConstructions().size() << ","
         << "chunksExplored: " << user.exploration().numChunksExplored() << ","
         << "chunksTotal: " << user.exploration().numChunks() << ","
         << "location: " << user.realWorldLocation() << ",";

  stream << "inventory: [";
  for (auto inventorySlot : user.inventory()) {
    auto id = inventorySlot.first != nullptr ? inventorySlot.first->id() : ""s;
    stream << "{id:\"" << id << "\", qty:" << inventorySlot.second << "},";
  }
  stream << "],";

  stream << "gear: [";
  for (auto gearSlot : user.gear()) {
    auto id = gearSlot.first != nullptr ? gearSlot.first->id() : ""s;
    stream << "{id:\"" << id << "\", qty:" << gearSlot.second << "},";
  }
  stream << "],";

  stream << "},";
}

static std::map<std::string, int> getUsersFromFiles() {
  // Name -> seconds offline
  std::map<std::string, int> users{};
  auto findData = WIN32_FIND_DATA{};
  auto handle = FindFirstFile("Users\\*.usr", &findData);
  if (handle != INVALID_HANDLE_VALUE) {
    BOOL fileFound = 1;
    while (fileFound) {
      auto name = std::string(findData.cFileName);
      name = name.substr(0, name.size() - 4);
      auto secondsOffline = toSeconds(findData.ftLastWriteTime);
      users[name] = secondsOffline;

      fileFound = FindNextFile(handle, &findData);
    }
  }

  return users;
}

void Server::publishStats(Server *server) {
  static auto publishingStats = false;
  if (publishingStats) return;
  publishingStats = true;
  ++server->_threadsOpen;

  server->debug()("Started logging");

  auto statsFile = std::ofstream{"logging/stats.js"};
  statsFile << "stats = {\n\n";

  statsFile << "version: \"" << version() << "\",\n";

  statsFile << "time: " << server->_time << ",\n";

  statsFile << "recipes: " << server->_recipes.size() << ",\n";
  statsFile << "constructions: " << server->_numBuildableObjects << ",\n";
  statsFile << "quests: " << server->_quests.size() << ",\n";

  statsFile << "users: [";

  // Online users
  for (const auto userEntry : server->_usersByName)
    server->writeUserToFile(*userEntry.second, statsFile);

  // Ofline users
  auto offlineUsers = getUsersFromFiles();
  for (const auto &offlineUser : offlineUsers) {
    auto userIsOnline = server->_usersByName.find(offlineUser.first) !=
                        server->_usersByName.end();
    if (userIsOnline) continue;

    auto user = User{offlineUser.first, {}, nullptr};
    server->readUserData(user, false);
    user.secondsOffline = offlineUser.second;

    server->writeUserToFile(user, statsFile);
  }

  statsFile << "\n],\n";

  statsFile << "\n};\n";

  server->debug()("Finished logging");

  --server->_threadsOpen;
  publishingStats = false;
}
