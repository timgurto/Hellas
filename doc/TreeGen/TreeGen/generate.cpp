#include <SDL.h>
#include <SDL_image.h>
#include <Windows.h>
#include <XmlReader.h>

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>

#include "../../../src/Stats.h"
#include "Edge.h"
#include "JsonWriter.h"
#include "Node.h"
#include "Recipe.h"
#include "SoundProfile.h"
#include "types.h"

using namespace std::string_literals;

std::map<std::string, CompositeStat> Stats::compositeDefinitions;

struct Path {
  std::string child;
  std::vector<std::string> parents;

  Path(const std::string &startNode) : child(startNode) {
    parents.push_back(startNode);
  }
};

bool checkImageExists(std::string filename);

#undef main
int main(int argc, char **argv) {
  std::set<Edge> edges;
  std::map<Tag, Node::Name> tools;
  Nodes nodes;
  std::set<Edge> blacklist, extras;
  std::map<std::string, std::string> collapses;
  std::map<std::string, std::string> tagNames;

  struct NpcTemplate {
    std::string imageFile;
    std::string soundProfile;
  };
  std::map<std::string, NpcTemplate> npcTemplates;

  std::string colorScheme = "rdylgn6";
  std::map<EdgeType, size_t> edgeColors;
  // Weak: light colors
  edgeColors[UNLOCK_ON_ACQUIRE] = 1;  // Weak, since you can trade for the item.
  edgeColors[GATHER] = 1;
  edgeColors[CONSTRUCT_FROM_ITEM] = 2;
  edgeColors[UNLOCK_ON_GATHER] = 3;
  edgeColors[LOOT] = 4;
  edgeColors[GATHER_REQ] = 5;
  edgeColors[CONSTRUCTION_REQ] = 5;
  // Strong: dark colors
  edgeColors[TRANSFORM] = 6;
  edgeColors[UNLOCK_ON_CONSTRUCT] = 6;
  edgeColors[UNLOCK_ON_CRAFT] = 6;

  const std::string dataPath = "../../Data";

  using FilesList = std::set<std::string>;
  auto filesList = FilesList{};
  WIN32_FIND_DATA fd;
  auto path = dataPath + "/";
  std::replace(path.begin(), path.end(), '/', '\\');
  std::string filter = path + "*.xml";
  path.c_str();
  HANDLE hFind = FindFirstFile(filter.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (fd.cFileName == std::string{"map.xml"}) continue;
      auto file = path + fd.cFileName;
      filesList.insert(file);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
  }

  // Load tools
  auto xr = XmlReader::FromFile("archetypalTools.xml");
  for (auto elem : xr.getChildren("tool")) {
    std::string archetype;
    Tag tag;
    if (!xr.findAttr(elem, "id", tag)) continue;
    if (!xr.findAttr(elem, "archetype", archetype)) continue;
    tools[tag] = archetype;
  }

  // Load blacklist
  for (auto elem : xr.getChildren("blacklist")) {
    std::string parent, child;
    if (!xr.findAttr(elem, "parent", parent)) {
      std::cout << "Blacklist item had no parent; ignored" << std::endl;
      continue;
    }
    if (!xr.findAttr(elem, "child", child)) {
      std::cout << "Blacklist item had no child; ignored" << std::endl;
      continue;
    }
    blacklist.insert(Edge(parent, child, DEFAULT));
  }

  // Load collapses
  for (auto elem : xr.getChildren("collapse")) {
    std::string id, into;
    if (!xr.findAttr(elem, "id", id)) {
      std::cout << "Collapse item had no source; ignored" << std::endl;
      continue;
    }
    if (!xr.findAttr(elem, "into", into)) {
      std::cout << "Collapse item had no target; ignored" << std::endl;
      continue;
    }
    collapses[id] = into;
  }

  // Load extra edges
  for (auto elem : xr.getChildren("extra")) {
    std::string parent, child;
    if (!xr.findAttr(elem, "parent", parent)) {
      std::cout << "Extra edge had no parent; ignored" << std::endl;
      continue;
    }
    if (!xr.findAttr(elem, "child", child)) {
      std::cout << "Extra edge had no child; ignored" << std::endl;
      continue;
    }
    Edge e(parent, child, DEFAULT);
    extras.insert(e);
  }

  // Load sound profiles
  std::map<ID, SoundProfile> soundProfiles;
  for (auto file : filesList) {
    xr.newFile(file);
    for (auto elem : xr.getChildren("soundProfile")) {
      ID id;
      if (!xr.findAttr(elem, "id", id)) continue;
      SoundProfile profile;
      for (auto sound : xr.getChildren("sound", elem)) {
        std::string soundType;
        if (xr.findAttr(sound, "type", soundType)) profile.addType(soundType);
      }
      soundProfiles[id] = profile;
    }
  }

  // Load terrain
  std::map<ID, size_t> terrain;
  for (auto file : filesList) {
    xr.newFile(file);
    for (auto elem : xr.getChildren("terrain")) {
      ID id;
      if (!xr.findAttr(elem, "id", id)) continue;

      size_t frames = 1;
      xr.findAttr(elem, "frames", frames);

      terrain[id] = frames;
    }
  }

  // Load recipes
  std::map<ID, Recipe> recipes;
  std::map<ID, std::set<std::string>> materialFor, toolFor;
  JsonWriter jw("recipesWithStuffMissing");
  for (auto file : filesList) {
    xr.newFile(file);
    for (auto elem : xr.getChildren("recipe")) {
      ID recipeID;
      if (!xr.findAttr(elem, "id", recipeID)) continue;
      ID product = recipeID;
      xr.findAttr(elem, "product", product);
      std::string label = "item_" + product;

      Recipe r;

      for (auto material : xr.getChildren("material", elem)) {
        ID matID;
        if (!xr.findAttr(material, "id", matID)) continue;
        std::string qty = "1";
        xr.findAttr(material, "quantity", qty);
        std::string matEntry = "{id:\"" + matID + "\", quantity:" + qty + "}";
        r.ingredients.insert(matEntry);
        materialFor[matID].insert(product);
      }

      for (auto material : xr.getChildren("tool", elem)) {
        ID tool;
        if (!xr.findAttr(material, "class", tool)) continue;
        r.tools.insert(tool);
        toolFor[tool].insert(product);
        tagNames[tool] = tool;
      }

      std::string s;
      if (xr.findAttr(elem, "time", s)) r.time = s;
      if (xr.findAttr(elem, "quantity", s)) r.quantity = s;

      ID id;
      for (auto unlockBy : xr.getChildren("unlockedBy", elem)) {
        double chance = 1.0;
        xr.findAttr(unlockBy, "chance", chance);
        if (xr.findAttr(unlockBy, "recipe", id) ||
            xr.findAttr(unlockBy, "item", id))
          edges.insert(Edge("item_" + id, label, UNLOCK_ON_CRAFT, chance));
        else if (xr.findAttr(unlockBy, "construction", id))
          edges.insert(
              Edge("object_" + id, label, UNLOCK_ON_CONSTRUCT, chance));
        else if (xr.findAttr(unlockBy, "gather", id))
          edges.insert(Edge("item_" + id, label, UNLOCK_ON_GATHER, chance));
        else if (xr.findAttr(unlockBy, "item", id))
          edges.insert(Edge("item_" + id, label, UNLOCK_ON_ACQUIRE, chance));
      }

      ID sounds;
      if (xr.findAttr(elem, "sounds", sounds)) {
        if (!sounds.empty()) {
          SoundProfile &soundProfile = soundProfiles[sounds];
          soundProfile.checkType("crafting");
        }
      } else {
        jw.nextEntry();
        jw.addAttribute("id", recipeID);
        jw.addArrayAttribute("soundsMissing", {"crafting"});
      }

      recipes[product] = r;
    }
  }

  // Crafting Tools
  {
    JsonWriter jw("tools");
    for (const auto &pair : toolFor) {
      jw.nextEntry();
      jw.addAttribute("tag", pair.first);
      jw.addArrayAttribute("crafting", pair.second);
    }
  }

  // Load items
  std::set<std::string> objectsConstructedFromItems;
  {
    JsonWriter jw("items");
    for (auto file : filesList) {
      xr.newFile(file);
      for (auto elem : xr.getChildren("item")) {
        jw.nextEntry();
        ID id;
        if (!xr.findAttr(elem, "id", id)) continue;
        Node::Name name = "item_" + id;

        auto iconFile = id;
        xr.findAttr(elem, "iconFile", iconFile);
        jw.addAttribute("image", "item_" + iconFile);
        jw.addAttribute("id", id);

        auto recipeIt = recipes.find(id);
        if (recipeIt != recipes.end()) recipeIt->second.writeToJSON(jw);

        auto usedAsMatIt = materialFor.find(id);
        if (usedAsMatIt != materialFor.end())
          jw.addArrayAttribute("usedAsMaterial", usedAsMatIt->second);

        std::set<ID> requiredSounds;
        requiredSounds.insert("drop");
        std::set<ID> missingImages;

        if (!iconFile.empty() && !checkImageExists("Items/" + iconFile))
          missingImages.insert("icon");

        std::string s;
        if (xr.findAttr(elem, "name", s)) {
          nodes.add(Node(ITEM, id, s, iconFile));
          jw.addAttribute("name", s);
        }

        if (xr.findAttr(elem, "constructs", s)) {
          edges.insert(Edge(name, "object_" + s, CONSTRUCT_FROM_ITEM));
          jw.addAttribute("constructs", s);
          objectsConstructedFromItems.insert(s);
        }

        std::set<std::string> tags;
        for (auto tag : xr.getChildren("tag", elem))
          if (xr.findAttr(tag, "name", s)) {
            tags.insert(s);
            tagNames[s] = s;
          }
        jw.addArrayAttribute("tags", tags);

        if (xr.findAttr(elem, "stackSize", s)) jw.addAttribute("stackSize", s);
        if (xr.findAttr(elem, "gearSlot", s)) {
          jw.addAttribute("gearSlot", s);
          bool isWeapon = (s == "6");
          bool isArmor = (s == "0" || s == "2" || s == "3" || s == "4" ||
                          s == "5" || s == "7");
          if (isWeapon)
            requiredSounds.insert("attack");
          else if (isArmor)
            requiredSounds.insert("defend");

          auto gearFile = id;
          xr.findAttr(elem, "gearFile", gearFile);
          if (!gearFile.empty() && !checkImageExists("Gear/" + gearFile))
            missingImages.insert("gear");
        }

        for (auto stat : xr.getChildren("stats", elem)) {
          if (xr.findAttr(stat, "health", s)) jw.addAttribute("health", s);
          if (xr.findAttr(stat, "attack", s)) jw.addAttribute("attack", s);
          if (xr.findAttr(stat, "speed", s)) jw.addAttribute("speed", s);
          if (xr.findAttr(stat, "attackTime", s))
            jw.addAttribute("attackTime", s);
        }

        ID sounds;
        if (xr.findAttr(elem, "sounds", sounds)) {
          if (!sounds.empty()) {
            SoundProfile &soundProfile = soundProfiles[sounds];
            for (const std::string &soundType : requiredSounds)
              soundProfile.checkType(soundType);
          }
        } else {
          jw.addArrayAttribute("soundsMissing", requiredSounds);
        }

        if (!missingImages.empty())
          jw.addArrayAttribute("imagesMissing", missingImages);
      }
    }
  }

  // Load objects
  {
    JsonWriter jw("objects");
    for (auto file : filesList) {
      xr.newFile(file);
      for (auto elem : xr.getChildren("objectType")) {
        jw.nextEntry();
        ID id;
        if (!xr.findAttr(elem, "id", id)) continue;
        Node::Name name = "object_" + id;
        jw.addAttribute("id", id);

        std::string displayName = id;
        xr.findAttr(elem, "name", displayName);
        Node node(OBJECT, id, displayName);
        jw.addAttribute("name", displayName);

        ID image = id;
        if (xr.findAttr(elem, "imageFile", image)) {
          jw.addAttribute("image", "object_" + image);
          node.customImage(image);
        } else
          jw.addAttribute("image", name);

        nodes.add(node);

        std::set<ID> requiredSounds, missingParticles, missingImages;

        bool canBeOwned = false;
        if (objectsConstructedFromItems.find(id) !=
            objectsConstructedFromItems.end())
          canBeOwned = true;

        if (!checkImageExists("Objects/" + image))
          missingImages.insert("normal");

        ID gatherReq;
        std::string s;
        if (xr.findAttr(elem, "gatherReq", gatherReq)) {
          auto it = tools.find(gatherReq);
          if (it == tools.end()) {
            std::cerr << "Tool class is missing archetype: " << gatherReq
                      << std::endl;
            edges.insert(Edge(gatherReq, name, GATHER_REQ));
          } else
            edges.insert(Edge(it->second, name, GATHER_REQ));
          jw.addAttribute("gatherReq", s);
          tagNames[s] = s;
        }

        if (xr.findAttr(elem, "constructionReq", s)) {
          canBeOwned = true;
          auto it = tools.find(s);
          if (it == tools.end()) {
            std::cerr << "Tool class is missing archetype: " << s << std::endl;
            edges.insert(Edge(s, name, CONSTRUCTION_REQ));
          } else
            edges.insert(Edge(it->second, name, CONSTRUCTION_REQ));
          jw.addAttribute("constructionReq", s);
          tagNames[s] = s;
        }

        std::set<std::string> yields;
        for (auto yield : xr.getChildren("yield", elem)) {
          if (!xr.findAttr(yield, "id", s)) continue;
          edges.insert(Edge(name, "item_" + s, GATHER));
          yields.insert(s);
        }
        if (!yields.empty()) {
          requiredSounds.insert("gather");
          if (!xr.findAttr(elem, "gatherParticles", s))
            missingParticles.insert("gather");
        }
        jw.addArrayAttribute("yield", yields);

        std::set<std::string> unlocksForJson;
        for (auto unlockBy : xr.getChildren("unlockedBy", elem)) {
          double chance = 1.0;
          xr.findAttr(unlockBy, "chance", chance);
          if (xr.findAttr(unlockBy, "recipe", s) ||
              xr.findAttr(unlockBy, "item", s)) {
            edges.insert(Edge("item_" + s, name, UNLOCK_ON_CRAFT, chance));
            unlocksForJson.insert("{type:\"craft\", sourceID:\"" + s + "\"}");
          } else if (xr.findAttr(unlockBy, "construction", s)) {
            edges.insert(
                Edge("object_" + s, name, UNLOCK_ON_CONSTRUCT, chance));
            unlocksForJson.insert("{type:\"construct\", sourceID:\"" + s +
                                  "\"}");
          } else if (xr.findAttr(unlockBy, "gather", s)) {
            edges.insert(Edge("item_" + s, name, UNLOCK_ON_GATHER, chance));
            unlocksForJson.insert("{type:\"gather\", sourceID:\"" + s + "\"}");
          } else if (xr.findAttr(unlockBy, "item", s)) {
            edges.insert(Edge("item_" + s, name, UNLOCK_ON_ACQUIRE, chance));
            unlocksForJson.insert("{type:\"acquire\", sourceID:\"" + s + "\"}");
          }
        }
        jw.addArrayAttribute("unlockedBy", unlocksForJson, true);

        std::set<std::string> materialsForJson;
        for (auto material : xr.getChildren("material", elem)) {
          std::string quantity = "1";
          if (!xr.findAttr(material, "id", s)) continue;
          xr.findAttr(material, "quantity", quantity);  // Default = 1, above.
          materialsForJson.insert("{id:\"" + s + "\", quantity:" + quantity +
                                  "}");
          // Add edge for construction material, if there is no existing edge.
          // Probably useful only for debugging purposes, or to prompt ideas for
          // <extra> edges.
          // auto materialEdge = Edge{"item_" + s, name, INGREDIENT};
          // if (edges.count(materialEdge) == 0) edges.insert(materialEdge);
        }
        jw.addArrayAttribute("materials", materialsForJson, true);
        if (!materialsForJson.empty()) {
          if (!checkImageExists("Objects/" + image + "-construction"))
            missingImages.insert("construction");
          canBeOwned = true;
        }

        auto transform = xr.findChild("transform", elem);
        if (transform && xr.findAttr(transform, "id", s)) {
          edges.insert(Edge(name, "object_" + s, TRANSFORM));
          jw.addAttribute("transformID", s);
          if (xr.findAttr(transform, "time", s))
            jw.addAttribute("transformTime", s);
        }

        if (xr.findAttr(elem, "gatherTime", s))
          jw.addAttribute("gatherTime", s);
        if (xr.findAttr(elem, "constructionTime", s))
          jw.addAttribute("constructionTime", s);
        if (xr.findAttr(elem, "merchantSlots", s))
          jw.addAttribute("merchantSlots", s);
        if (xr.findAttr(elem, "bottomlessMerchant", s) && s != "0")
          jw.addAttribute("bottomlessMerchant", "true");
        if (xr.findAttr(elem, "isVehicle", s) && s != "0")
          jw.addAttribute("isVehicle", "true");
        if (xr.findAttr(elem, "deconstructs", s))
          jw.addAttribute("deconstructs", s);
        if (xr.findAttr(elem, "deconstructionTime", s))
          jw.addAttribute("deconstructionTime", s);

        std::set<std::string> tags;
        for (auto tag : xr.getChildren("tag", elem))
          if (xr.findAttr(tag, "name", s)) {
            tags.insert(s);
            tagNames[s] = s;
          }
        jw.addArrayAttribute("tags", tags);

        auto container = xr.findChild("container", elem);
        if (container && xr.findAttr(container, "slots", s))
          jw.addAttribute("containerSlots", s);

        if (canBeOwned) {
          if (!checkImageExists("Objects/" + image + "-corpse"))
            missingImages.insert("corpse");
          if (!xr.findAttr(elem, "damageParticles", s))
            missingParticles.insert("damage");

          requiredSounds.insert("defend");
          requiredSounds.insert("death");
        }

        if (!requiredSounds.empty()) {
          ID soundProfileID;
          if (xr.findAttr(elem, "sounds", soundProfileID)) {
            SoundProfile &soundProfile = soundProfiles[soundProfileID];
            for (const std::string &soundType : requiredSounds)
              soundProfile.checkType(soundType);
          } else
            jw.addArrayAttribute("soundsMissing", requiredSounds);
        }

        if (!missingParticles.empty())
          jw.addArrayAttribute("particlesMissing", missingParticles);

        if (!missingImages.empty())
          jw.addArrayAttribute("imagesMissing", missingImages);
      }
    }
  }

  // Load spells
  {
    JsonWriter jw("spells");
    for (auto file : filesList) {
      xr.newFile(file);
      for (auto elem : xr.getChildren("spell")) {
        jw.nextEntry();

        std::string displayName;
        if (xr.findAttr(elem, "name", displayName)) {
          jw.addAttribute("name", displayName);
        }

        std::set<ID> requiredSounds, missingParticles;
        requiredSounds.insert("impact");

        auto aesthetics = xr.findChild("aesthetics", elem);
        if (!aesthetics) {
          missingParticles.insert("impact");
        } else {
          if (xr.findAttr(aesthetics, "projectile", std::string{}))
            requiredSounds.insert("launch");
          if (!xr.findAttr(aesthetics, "impactParticles", std::string{}))
            missingParticles.insert("impact");

          ID soundProfileID;
          if (xr.findAttr(aesthetics, "sounds", soundProfileID)) {
            SoundProfile &soundProfile = soundProfiles[soundProfileID];
            for (const std::string &soundType : requiredSounds)
              soundProfile.checkType(soundType);
          } else
            jw.addArrayAttribute("soundsMissing", requiredSounds);
        }

        if (!missingParticles.empty())
          jw.addArrayAttribute("particlesMissing", missingParticles);
      }
    }
  }

  // Load NPC templates
  for (auto file : filesList) {
    xr.newFile(file);

    for (auto elem : xr.getChildren("npcTemplate")) {
      ID id, imageFile;
      if (!xr.findAttr(elem, "id", id)) continue;
      if (!xr.findAttr(elem, "imageFile", imageFile)) continue;
      auto &nt = npcTemplates[id];
      nt.imageFile = imageFile;
      xr.findAttr(elem, "sounds", nt.soundProfile);
    }
  }

  // Load NPCs
  {
    JsonWriter jw("npcs");
    for (auto file : filesList) {
      xr.newFile(file);

      for (auto elem : xr.getChildren("npcType")) {
        std::set<ID> requiredSounds, missingImages, missingParticles;
        requiredSounds.insert("attack");
        requiredSounds.insert("defend");
        requiredSounds.insert("death");

        jw.nextEntry();
        ID id;
        if (!xr.findAttr(elem, "id", id)) continue;
        jw.addAttribute("id", id);
        Node::Name name = "npc_" + id;

        auto isCivilian = 0;
        if (xr.findAttr(elem, "isCivilian", isCivilian) && isCivilian) {
          requiredSounds.erase("attack");
          requiredSounds.erase("defend");
          requiredSounds.erase("death");
        }

        auto imageID = id;
        auto templateID = ""s;
        if (xr.findAttr(elem, "template", templateID)) {
          imageID = npcTemplates[templateID].imageFile;
        }

        xr.findAttr(elem, "imageFile", imageID);
        auto humanoid = xr.findChild("humanoid", elem);
        if (humanoid) {
          imageID = "default";
          xr.findAttr(humanoid, "base", imageID);
        }
        jw.addAttribute("image", "npc_" + imageID);
        auto directory = humanoid ? "Humans/"s : "NPCs/"s;
        if (!checkImageExists(directory + imageID))
          missingImages.insert("normal");
        if (!isCivilian && !checkImageExists(directory + imageID + "-corpse"))
          missingImages.insert("corpse");

        std::string displayName;
        if (xr.findAttr(elem, "name", displayName)) {
          nodes.add(Node(NPC, id, displayName, imageID));
          jw.addAttribute("name", displayName);
        }

        ID soundProfileID;
        if (humanoid)
          soundProfileID = "humanEnemy";
        else if (!templateID.empty())
          soundProfileID = npcTemplates[templateID].soundProfile;
        xr.findAttr(elem, "sounds", soundProfileID);
        if (soundProfileID.empty())
          jw.addArrayAttribute("soundsMissing", requiredSounds);
        else {
          SoundProfile &soundProfile = soundProfiles[soundProfileID];
          soundProfile.checkType("attack");
          soundProfile.checkType("defend");
          soundProfile.checkType("death");
        }

        std::set<ID> lootList;
        for (auto loot : xr.getChildren("loot", elem)) {
          ID lootItem;
          if (!xr.findAttr(loot, "id", lootItem)) continue;
          // edges.insert(Edge(name, "item_" + lootItem, LOOT));
          lootList.insert(lootItem);
        }
        if (!lootList.empty()) jw.addArrayAttribute("loot", lootList);

        std::set<std::string> yields;
        auto gatherItem = ID{};
        for (auto yield : xr.getChildren("yield", elem)) {
          if (!xr.findAttr(yield, "id", gatherItem)) continue;
          edges.insert(Edge(name, "item_" + gatherItem, GATHER));
          yields.insert(gatherItem);
        }
        if (!yields.empty()) {
          requiredSounds.insert("gather");
          auto dummy = ""s;
          if (!xr.findAttr(elem, "gatherParticles", dummy))
            missingParticles.insert("gather");
        }
        jw.addArrayAttribute("yield", yields);

        std::string stat;
        if (xr.findAttr(elem, "maxHealth", stat))
          jw.addAttribute("health", stat);
        if (xr.findAttr(elem, "attack", stat)) jw.addAttribute("attack", stat);
        if (xr.findAttr(elem, "attackTime", stat))
          jw.addAttribute("attackTime", stat);

        if (!missingImages.empty())
          jw.addArrayAttribute("imagesMissing", missingImages);
        if (!missingParticles.empty())
          jw.addArrayAttribute("particlesMissing", missingParticles);
      }
    }
  }

  // Load tag names
  for (auto file : filesList) {
    xr.newFile(file);
    for (auto elem : xr.getChildren("tag")) {
      ID id;
      if (!xr.findAttr(elem, "id", id)) continue;

      std::string name;
      if (!xr.findAttr(elem, "name", name)) continue;

      tagNames[id] = name;
    }
  }

  // Write tag names to JSON
  {
    JsonWriter jw("tags");
    for (auto &pair : tagNames) {
      jw.nextEntry();

      jw.addAttribute("id", pair.first);
      jw.addAttribute("name", pair.second);
    }
  }

  // Write sound profiles to JSON
  {
    JsonWriter jw("soundProfiles");
    for (const auto &pair : soundProfiles) {
      jw.nextEntry();
      jw.addAttribute("id", pair.first);
      const SoundProfile &profile = pair.second;
      jw.addArrayAttribute("missingTypes", profile.getMissingTypes());
    }
  }

  // Write missing terrain to JSON
  {
    JsonWriter jw("terrains");
    for (const auto &pair : terrain) {
      // Check that image(s) exists
      if (pair.second == 1) {
        if (!checkImageExists("Terrain/" + pair.first)) {
          jw.nextEntry();
          jw.addAttribute("image", pair.first);
        }
      } else {
        for (size_t i = 0; i != pair.second; ++i) {
          std::ostringstream filename;
          filename << pair.first;
          if (i < 10) filename << "0";
          filename << i;

          if (!checkImageExists("Terrain/" + filename.str())) {
            jw.nextEntry();
            jw.addAttribute("image", filename.str());
          }
        }
      }
    }
  }

  // Generate images, before nodes are deleted
  nodes.generateAllImages();

  // Collapse nodes
  {
    std::set<Edge> newEdges;
    for (auto it = edges.begin(); it != edges.end();) {
      auto nextIt = it;
      ++nextIt;
      bool edgeNeedsReplacing = false;
      Edge newEdge = *it;
      if (collapses.find(it->parent) != collapses.end()) {
        newEdge.parent = collapses[newEdge.parent];
        edgeNeedsReplacing = true;
      }
      if (collapses.find(it->child) != collapses.end()) {
        newEdge.child = collapses[newEdge.child];
        edgeNeedsReplacing = true;
      }
      if (edgeNeedsReplacing) {
        std::cout << "Collapsing edge:" << std::endl
                  << "            " << *it << std::endl
                  << "    becomes " << newEdge << std::endl;
        newEdges.insert(newEdge);
        edges.erase(it);
      }
      it = nextIt;
    }
    for (const Edge &newEdge : newEdges) {
      edges.insert(newEdge);
    }
  }
  std::set<Node::Name> nodesToRemove;
  for (const auto &collapse : collapses) nodesToRemove.insert(collapse.first);
  for (const Node::Name &name : nodesToRemove) nodes.remove(name);

  // Remove blacklisted items
  for (auto blacklistedEdge : blacklist) {
    for (auto it = edges.begin(); it != edges.end(); ++it)
      if (*it == blacklistedEdge) {
        std::cout << "Removing blacklisted edge: " << *it << std::endl;
        edges.erase(it);
        break;
      }
  }

  // Remove self-references
  for (auto edgeIt = edges.begin(); edgeIt != edges.end();) {
    auto nextIt = edgeIt;
    ++nextIt;
    if (edgeIt->parent == edgeIt->child) {
      std::cout << "Removing self-referential edge " << *edgeIt << std::endl;
      edges.erase(edgeIt);
    }
    edgeIt = nextIt;
  }

  // Remove shortcuts
  for (auto edgeIt = edges.begin(); edgeIt != edges.end();) {
    /*
    This is one edge.  We want to figure out if there's a longer path here
    (i.e., this path doesn't add any new information about progress
    requirements).
    */
    bool shouldDelete = false;
    std::queue<std::string> queue;
    std::set<std::string> nodesFound;

    // Populate queue initially with all direct children
    for (const Edge &edge : edges)
      if (edge.parent == edgeIt->parent && edge.child != edgeIt->child)
        queue.push(edge.child);

    // New nodes to check will be added here.  Effectively it will be a
    // breadth-first search.
    while (!queue.empty() && !shouldDelete) {
      std::string nextParent = queue.front();
      queue.pop();
      for (const Edge &edge : edges) {
        if (edge.parent != nextParent) continue;
        if (edge.child == edgeIt->child) {
          // Mark the edge for removal
          std::cout << "Removing shortcut: " << edge << std::endl;
          shouldDelete = true;
          break;
        } else if (nodesFound.find(edge.child) != nodesFound.end()) {
          queue.push(edge.child);
          nodesFound.insert(edge.child);
        }
      }
    }
    auto nextIt = edgeIt;
    ++nextIt;
    if (shouldDelete) edges.erase(edgeIt);
    edgeIt = nextIt;
  }

  // Remove loops
  // First, find set of nodes that start edges but don't finish them (i.e.,
  // the roots of the tree).
  std::set<std::string> starts, ends;
  for (auto &edge : edges) {
    starts.insert(edge.parent);
    ends.insert(edge.child);
  }
  for (const std::string &endNode : ends) {
    starts.erase(endNode);
  }

  // Next, do a BFS from each to find loops.  Remove those final edges.
  std::set<Edge> trashCan;
  for (const std::string &startNode : starts) {
    // std::cout << "Root node: " << startNode << std::endl;
    std::queue<Path> queue;
    queue.push(Path(startNode));
    bool loopFound = false;
    while (!queue.empty() && !loopFound) {
      Path nextPath = queue.front();
      queue.pop();
      for (auto it = edges.begin(); it != edges.end(); ++it) {
        if (it->parent != nextPath.child) continue;
        std::string child = it->child;
        // If this child is already a parent
        if (std::find(nextPath.parents.begin(), nextPath.parents.end(),
                      child) != nextPath.parents.end()) {
          std::cout << "Loop found; marked for removal:" << std::endl << "  ";
          for (const std::string &parent : nextPath.parents)
            std::cout << parent << " -> ";
          std::cout << child << std::endl;
          // Mark the edge for removal
          trashCan.insert(*it);
        } else {
          Path p = nextPath;
          p.parents.push_back(child);
          p.child = child;
          queue.push(p);
        }
      }
    }
    for (auto edgeToDelete : trashCan)
      for (auto it = edges.begin(); it != edges.end(); ++it)
        if (*it == edgeToDelete) {
          edges.erase(edgeToDelete);
          break;
        }
  }

  // Publish
  std::ofstream f("tree.gv");
  f << "digraph techTree {" << std::endl;
  f << "bgcolor=\"#ffffff00\"" << std::endl;
  f << "node [fontsize=10 fontname=\"Advocut\" imagescale=true "
       "fontcolor=\"#999999\" color=\"#999999\"];"
    << std::endl;
  f << "edge [fontsize=10 fontname=\"Advocut\" fontcolor=\"#999999\" "
       "color=\"#999999\"];"
    << std::endl;

  auto nodesWithConnections = std::set<std::string>{};
  for (auto &edge : edges) {
    nodesWithConnections.insert(edge.parent);
    nodesWithConnections.insert(edge.child);
  }
  for (auto &edge : extras) {
    nodesWithConnections.insert(edge.parent);
    nodesWithConnections.insert(edge.child);
  }

  // Nodes
  nodes.outputAsGraphviz(f, nodesWithConnections);

  // Edges
  for (auto &edge : edges) {
    f << edge.parent << " -> " << edge.child << " [colorscheme=\""
      << colorScheme << "\", color=" << edgeColors[edge.type];
    if (edge.chance < 1.0)
      f << " label=\"" << static_cast<int>(edge.chance * 100 + 0.5) << "%\"";
    f << "]" << std::endl;
  }

  // Extra edges
  for (auto &edge : extras)
    f << edge.parent << " -> " << edge.child
      << " [style=dotted constraint=true]" << std::endl;

  f << "}";

  return 0;
}

bool checkImageExists(std::string filename) {
  filename = "../../Images/" + filename + ".png";
  SDL_Surface *surface = IMG_Load(filename.c_str());
  if (surface == nullptr) return false;
  SDL_FreeSurface(surface);
  return true;
}
