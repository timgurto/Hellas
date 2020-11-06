#include "XmlReader.h"

#include <set>
#include <sstream>

#include "Stats.h"

#ifndef NO_SDL
#include "NormalVariable.h"
#include "Rect.h"
#include "server/ServerItem.h"
#endif  // NO_SDL

XmlReader::XmlReader(const std::string &string, bool isFile) : _root(nullptr) {
  if (isFile)
    newFile(string);
  else {
    newString("<root>" + string + "</root>\n");
  }
}

XmlReader XmlReader::FromString(const std::string &data) {
  return XmlReader(data, false);
}

XmlReader XmlReader::FromFile(const std::string &filename) {
  return XmlReader(filename, true);
}

XmlReader::~XmlReader() { _doc.Clear(); }

bool XmlReader::newString(const std::string &data) {
  _doc.Clear();
  _root = nullptr;
  if (!_doc.Parse(data.c_str() /*, 0, TIXML_ENCODING_UTF8*/)) return false;
  _root = _doc.FirstChildElement();
  return *this;
}

bool XmlReader::newFile(const std::string &filename) {
  _doc.Clear();
  _root = nullptr;
  if (!_doc.LoadFile(filename)) return false;
  _root = _doc.FirstChildElement();
  return *this;
}

XmlReader::Elements XmlReader::getChildren(const std::string &val,
                                           TiXmlElement *elem) {
  if (elem == nullptr) return {};

  auto children = Elements{};
  for (TiXmlElement *child = elem->FirstChildElement(); child;
       child = child->NextSiblingElement())
    if (val == child->Value()) children.push_back(child);
  return children;
}

TiXmlElement *XmlReader::findChild(const std::string &val, TiXmlElement *elem) {
  if (elem == nullptr) return nullptr;

  for (TiXmlElement *child = elem->FirstChildElement(); child;
       child = child->NextSiblingElement())
    if (val == child->Value()) return child;
  return nullptr;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr,
                         std::string &val) {
  if (elem == nullptr) return false;

  const char *const cStrVal = elem->Attribute(attr);
  if (cStrVal != nullptr) {
    val = cStrVal;
    return true;
  }
  return false;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr,
                         BasisPoints &val) {
  auto rawNumber = short{};
  if (!findAttr(elem, attr, rawNumber)) return false;
  val = {rawNumber};
  return true;
}

#ifndef NO_SDL
bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, Color &val) {
  if (elem == nullptr) return false;

  const char *const cStrVal = elem->Attribute(attr);
  if (cStrVal != nullptr) {
    std::istringstream iss(cStrVal);
    Uint32 color;
    iss >> std::hex >> color;
    Uint8 b = color % 0x100, g = color >> 8 % 0x100, r = color >> 16 % 100;
    val = Color(r, g, b);
    return true;
  }
  return false;
}

bool XmlReader::findNormVarChild(const std::string &name, TiXmlElement *elem,
                                 double &mean, double &sd) {
  TiXmlElement *child = XmlReader::findChild(name, elem);
  if (child == nullptr) return false;
  if (!XmlReader::findAttr(child, "mean", mean)) mean = 0;
  if (!XmlReader::findAttr(child, "sd", sd)) sd = 1;
  return true;
}
#endif  // NO_SDL

bool XmlReader::findStatsChild(const std::string &name, TiXmlElement *elem,
                               StatsMod &val) {
  TiXmlElement *child = XmlReader::findChild(name, elem);
  if (child == nullptr) return false;

  val = StatsMod{};
  XmlReader::findAttr(child, "armor", val.armor);
  XmlReader::findAttr(child, "armour", val.armor);
  XmlReader::findAttr(child, "health", val.maxHealth);
  XmlReader::findAttr(child, "energy", val.maxEnergy);
  XmlReader::findAttr(child, "hps", val.hps);
  XmlReader::findAttr(child, "eps", val.eps);

  XmlReader::findAttr(child, "hit", val.hit);
  XmlReader::findAttr(child, "crit", val.crit);
  XmlReader::findAttr(child, "critResist", val.critResist);
  XmlReader::findAttr(child, "dodge", val.dodge);
  XmlReader::findAttr(child, "block", val.block);

  XmlReader::findAttr(child, "blockValue", val.blockValue);
  XmlReader::findAttr(child, "magicDamage", val.magicDamage);
  XmlReader::findAttr(child, "physicalDamage", val.physicalDamage);
  XmlReader::findAttr(child, "healing", val.healing);
  XmlReader::findAttr(child, "airResist", val.airResist);
  XmlReader::findAttr(child, "earthResist", val.earthResist);
  XmlReader::findAttr(child, "fireResist", val.fireResist);
  XmlReader::findAttr(child, "waterResist", val.waterResist);
  XmlReader::findAttr(child, "followerLimit", val.followerLimit);
  XmlReader::findAttr(child, "speed", val.speed);
  XmlReader::findAttr(child, "gatherBonus", val.gatherBonus);
  XmlReader::findAttr(child, "unlockBonus", val.unlockBonus);

  auto n = 0;
  val.stuns = XmlReader::findAttr(child, "stuns", n) && n != 0;

  for (const auto &compositeStat : Stats::compositeDefinitions) {
    const auto &statName = compositeStat.first;

    // Do this via an int variable, instead of directly into the map, to avoid
    // populating it with 0s.  Composite stats are StatsMods, and therefore if
    // every StatsMod has an entry for each composite stat, then
    // Stats::modify(StatsMod) will result in an infinite loop.
    auto amount = 0;
    if (XmlReader::findAttr(child, statName.c_str(), amount))
      val.composites[statName] = amount;
  }

  return true;
}
