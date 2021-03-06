#include <ctime>
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

void Server::writeUserToFile(const User &user, std::ostream &stream) const {
  const auto isOnline = user.hasSocket();
  auto secondsOnlineOrOffline =
      isOnline ? user.secondsPlayedThisSession() : user.secondsOffline;

  const auto className =
      &user.getClass() ? user.getClass().type().id() : "None";

  stream << "\n{"
         << "name: \"" << user.name() << "\","
         << "online: " << user.hasSocket() << ","
         << "secondsPlayed: " << user.secondsPlayed() << ","
         << "secondsOnlineOrOffline: " << secondsOnlineOrOffline << ","
         << "class: \"" << className << "\","
         << "level: \"" << user.level() << "\","
         << "xp: \"" << user.xp() << "\","
         << "xpNeeded: \"" << user.XP_PER_LEVEL[user.level()] << "\","
         << "isInTutorial: " << user.isInTutorial() << ","
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
         << "chunksExplored: " << user.exploration.numChunksExplored() << ","
         << "chunksTotal: " << user.exploration.numChunks() << ","
         << "location: " << user.realWorldLocation() << ",";

  if (isOnline) stream << "ip: \"" << user.socket().ip() << "\",";

  stream << "inventory: [";
  for (auto inventorySlot : user.inventory()) {
    auto id =
        inventorySlot.first.hasItem() ? inventorySlot.first.type()->id() : ""s;
    stream << "{id:\"" << id << "\", qty:" << inventorySlot.second << "},";
  }
  stream << "],";

  stream << "gear: [";
  for (auto gearSlot : user.gear()) {
    auto id = gearSlot.first.hasItem() ? gearSlot.first.type()->id() : ""s;
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

void Server::publishStats() {
  static auto publishingStats = false;
  if (publishingStats) return;
  publishingStats = true;
  incrementThreadCount();

  std::ostringstream oss;

  oss << "stats = {\n\n";

  oss << "version: \"" << version() << "\",\n";

  oss << "uptime: " << _time << ",\n";
  oss << "time: " << time(nullptr) << ",\n";

  oss << "recipes: " << _recipes.size() << ",\n";
  oss << "constructions: " << _numBuildableObjects << ",\n";
  oss << "quests: " << _quests.size() << ",\n";

  oss << "users: [";

  // Online users
  for (const auto userEntry : _usersByName)
    writeUserToFile(*userEntry.second, oss);

  // Ofline users
  auto offlineUsers = getUsersFromFiles();
  for (const auto &offlineUser : offlineUsers) {
    auto userIsOnline =
        _usersByName.find(offlineUser.first) != _usersByName.end();
    if (userIsOnline) continue;

    auto user = User{offlineUser.first, {}, nullptr};
    readUserData(user, false);
    user.secondsOffline = offlineUser.second;

    writeUserToFile(user, oss);
  }

  oss << "\n],\n";

  oss << "\n};\n";

  std::ofstream{"logging/stats.js"} << oss.str();

  decrementThreadCount();
  publishingStats = false;
}

void Server::logNumberOfOnlineUsers() const {
  auto currentTime = time(nullptr);
  std::ofstream{"logging/onlinePlayers.csv", std::ofstream::app}
      << currentTime << "," << _usersByName.size() << std::endl;
}

void Server::generateDurabilityList() {
  auto os = std::ofstream{"durability.csv"};

  for (const auto &item : _items) {
    auto isGear = item.gearSlot() != User::GEAR_SLOTS;
    auto isTool = item.hasTags();

    if (!isTool && !isGear) continue;

    auto description = isGear ? "gear" : "tool";  // If both, just call it gear

    os << description << "," << item.id() << "," << item.durability()
       << std::endl;
  }

  for (const auto *object : _objectTypes) {
    if (object->classTag() == 'n') continue;
    object->initStrengthAndMaxHealth();
    os << "object," << object->id() << "," << object->baseStats().maxHealth
       << std::endl;
  }
}
