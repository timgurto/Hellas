#include "EntityType.h"

#include "../../../src/XmlReader.h"

void EntityType::load(Container& container,
                      const NPCTemplate::Container& npcTemplates,
                      const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);

  for (auto elem : xr.getChildren("objectType")) {
    auto et = EntityType{};
    et.category = OBJECT;

    auto id = std::string{};
    xr.findAttr(elem, "id", id);

    auto imageFile = id;
    xr.findAttr(elem, "imageFile", imageFile);
    et.image = {"../../Images/Objects/" + imageFile + ".png", Color::MAGENTA};
    et.drawRect.w = et.image.width();
    et.drawRect.h = et.image.height();

    xr.findRectChild("collisionRect", elem, et.collisionRect);

    xr.findAttr(elem, "xDrawOffset", et.drawRect.x);
    xr.findAttr(elem, "yDrawOffset", et.drawRect.y);
    container[id] = et;
  }

  for (auto elem : xr.getChildren("npcType")) {
    auto et = EntityType{};
    et.category = NPC;

    auto id = std::string{};
    xr.findAttr(elem, "id", id);

    auto imageFile = id;

    auto isHumanoid = xr.findChild("humanoid", elem);
    if (isHumanoid) {
      imageFile = {"../Humans/default"};
      et.collisionRect = {-5, -2, 10, 4};
      et.drawRect.x = -9;
      et.drawRect.y = -39;
    }

    auto templateID = std::string{};
    xr.findAttr(elem, "template", templateID);
    if (!templateID.empty()) {
      auto nt = npcTemplates.find(templateID)->second;
      et.drawRect = nt.drawRect;
      et.collisionRect = nt.collisionRect;
      et.image = nt.image;
    }

    xr.findAttr(elem, "imageFile", imageFile);
    if (!imageFile.empty()) {
      et.image = {"../../Images/NPCs/" + imageFile + ".png", Color::MAGENTA};
      et.drawRect.w = et.image.width();
      et.drawRect.h = et.image.height();
    }

    xr.findRectChild("collisionRect", elem, et.collisionRect);

    xr.findAttr(elem, "level", et.level);

    xr.findAttr(elem, "xDrawOffset", et.drawRect.x);
    xr.findAttr(elem, "yDrawOffset", et.drawRect.y);
    container[id] = et;
  }
}

void NPCTemplate::load(Container& container, const std::string& filename) {
  auto xr = XmlReader::FromFile(filename);
  for (auto elem : xr.getChildren("npcTemplate")) {
    auto nt = NPCTemplate{};

    auto id = std::string{};
    xr.findAttr(elem, "id", id);

    auto imageFile = id;

    xr.findAttr(elem, "imageFile", imageFile);
    nt.image = {"../../Images/NPCs/" + imageFile + ".png", Color::MAGENTA};
    nt.drawRect.w = nt.image.width();
    nt.drawRect.h = nt.image.height();

    xr.findRectChild("collisionRect", elem, nt.collisionRect);

    xr.findAttr(elem, "xDrawOffset", nt.drawRect.x);
    xr.findAttr(elem, "yDrawOffset", nt.drawRect.y);
    container[id] = nt;
  }
}
