#include <ctime>
#include <fstream>
#include <utility>

#include "../versionUtil.h"
#include "DroppedItem.h"
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
         << "\n\t"
         << "online: " << user.hasSocket() << ","
         << "secondsPlayed: " << user.secondsPlayed() << ","
         << "secondsOnlineOrOffline: " << secondsOnlineOrOffline << ","
         << "\n\t"
         << "class: \"" << className << "\","
         << "level: \"" << user.level() << "\","
         << "xp: \"" << user.xp() << "\","
         << "xpNeeded: \"" << user.XP_PER_LEVEL[user.level()] << "\","
         << "isInTutorial: " << user.isInTutorial() << ","
         << "x: \"" << user.location().x << "\","
         << "y: \"" << user.location().y << "\","
         << "city: \"" << _cities.getPlayerCity(user.name()) << "\","
         << "isKing: " << _kings.isPlayerAKing(user.name()) << ","
         << "\n\t"
         << "health: " << user.health() << ","
         << "maxHealth: " << user.stats().maxHealth << ","
         << "energy: " << user.energy() << ","
         << "maxEnergy: " << user.stats().maxEnergy << ","
         << "\n\t"
         << "knownRecipes: " << user.knownRecipes().size() << ","
         << "completedQuests: " << user.questsCompleted().size() << ","
         << "knownConstructions: " << user.knownConstructions().size() << ","
         << "chunksExplored: " << user.exploration.numChunksExplored() << ","
         << "chunksTotal: " << user.exploration.numChunks() << ",";

  stream << "\n\t"
         << "inventory: [";
  for (auto inventorySlot : user.inventory()) {
    auto id = inventorySlot.hasItem() ? inventorySlot.type()->id() : ""s;
    stream << "\"" << id << "\",";
  }
  stream << "],";

  stream << "\n\t"
         << "gear: [";
  for (auto gearSlot : user.gear()) {
    auto id = gearSlot.hasItem() ? gearSlot.type()->id() : ""s;
    stream << "\"" << id << "\",";
  }
  stream << "],";

  stream << "\n\t"
         << "location: " << user.realWorldLocation() << ",";

  stream << "},";
}

std::map<std::string, int> Server::getUsersFromFiles() {
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

void Server::publishGameData() {
  auto ofs = std::ofstream{"logging/gameData.js"};

  ofs << "items=[\n";
  for (const auto &item : _items) {
    ofs << "{";
    ofs << "id: \"" << item.id() << "\"";

    if (item.quality() != Item::COMMON)
      ofs << ", quality: \"" << static_cast<int>(item.quality()) << "\"";

    if (!item.iconFile().empty())
      ofs << ", iconFile: \"" << item.iconFile() << "\"";

    ofs << "},\n";
  }
  ofs << "];\n";
}

static void outputMetalWealth(std::ostream &os,
                              Server::ItemCounts &itemCounts) {
  struct MetalValue {
    std::string metal;
    double value;
  };
  auto itemValues = std::map<std::string, MetalValue>{
      {"tinBar", {"tin", 1}},          {"tinScrap", {"tin", .1}},
      {"tinCoin", {"tin", .01}},       {"copperBar", {"copper", 1}},
      {"copperScrap", {"copper", .1}}, {"copperCoin", {"copper", .01}},
      {"silverBar", {"silver", 1}},    {"silverScrap", {"silver", .1}},
      {"silverCoin", {"silver", .01}}};

  using MetalWealth = std::map<std::string, double>;
  MetalWealth metalWealth;
  for (auto pair : itemValues) {
    const auto metalType = pair.second.metal;
    const auto qty = itemCounts[pair.first];
    const auto itemValue = pair.second.value;
    metalWealth[metalType] += qty * itemValue;
  }

  os << "metalWealth:{";
  os << "tin:" << metalWealth["tin"] << ",";
  os << "copper:" << metalWealth["copper"] << ",";
  os << "silver:" << metalWealth["silver"] << ",";
  os << "}\n";
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

  // Game data
  oss << "recipes: " << _recipes.size() << ",\n";
  oss << "constructions: " << _numBuildableObjects << ",\n";
  oss << "quests: " << _quests.size() << ",\n";

  auto itemCounts = std::map<std::string, int>{};

  // World state
  auto numObjects = 0;
  oss << "objectLocations: [";
  for (const auto *entity : _entities) {
    if (!entity->excludedFromPersistentState()) {
      oss << "{x:" << entity->location().x << ",y:" << entity->location().y
          << "},";
      countItemsInObject(*entity, itemCounts);
      ++numObjects;
    }
  }
  oss << "],\n";
  oss << "objects: " << numObjects << ",\n";

  oss << "users: [";

  // Online users
  for (const auto userEntry : _onlineUsersByName) {
    writeUserToFile(*userEntry.second, oss);
    countItemsOnUser(*userEntry.second, itemCounts);
  }

  // Offline users
  auto offlineUsers = getUsersFromFiles();
  for (const auto &offlineUser : offlineUsers) {
    auto userIsOnline =
        _onlineUsersByName.find(offlineUser.first) != _onlineUsersByName.end();
    if (userIsOnline) continue;

    auto user = User{offlineUser.first, {}, nullptr};
    readUserData(user, false);
    user.secondsOffline = offlineUser.second;

    writeUserToFile(user, oss);
    countItemsOnUser(user, itemCounts);
  }

  oss << "\n],\n";

  // Item counts
  oss << "itemCounts: [";
  for (auto pair : itemCounts)
    oss << "{id:\"" << pair.first << "\",qty:" << pair.second << "},";
  oss << "],\n";

  // Metal wealth
  outputMetalWealth(oss, itemCounts);

  oss << "\n};\n";

  std::ofstream{"logging/stats.js"} << oss.str();

  decrementThreadCount();
  publishingStats = false;
}

void Server::logNumberOfOnlineUsers() const {
  auto currentTime = time(nullptr);
  std::ofstream{"logging/onlinePlayers.csv", std::ofstream::app}
      << currentTime << "," << _onlineUsersByName.size() << std::endl;
}

static void countItems(const ServerItem::vect_t &items,
                       Server::ItemCounts &itemCounts) {
  for (auto slot : items) {
    if (!slot.hasItem()) continue;

    const auto id = slot.type()->id();
    const auto qty = slot.quantity();
    itemCounts[id] += qty;
  }
}

void Server::countItemsOnUser(const User &user, ItemCounts &itemCounts) const {
  countItems(user.inventory(), itemCounts);
  countItems(user.gear(), itemCounts);
}

void Server::countItemsInObject(const Entity &entity,
                                ItemCounts &itemCounts) const {
  const auto *object = dynamic_cast<const Object *>(&entity);
  if (object) {
    if (object->hasContainer())
      countItems(object->container().raw(), itemCounts);
    return;
  }

  const auto *droppedItem = dynamic_cast<const DroppedItem *>(&entity);
  if (droppedItem) {
    droppedItem->addToItemCounts(itemCounts);
    return;
  }
}
