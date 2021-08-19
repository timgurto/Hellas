#ifndef XML_READER_H
#define XML_READER_H

#include <tinyxml.h>

#include <set>
#include <string>
#include <vector>

#ifndef NO_SDL
#include "Color.h"
#include "Rect.h"
#include "combatTypes.h"
class NormalVariable;
;
#endif  // NO_SDL

struct StatsMod;
class BasisPoints;

// Wrapper class for TinyXml functionality
class XmlReader {
  TiXmlDocument _doc;
  TiXmlElement *_root{nullptr};

  XmlReader(const std::string &string, bool isFile);

 public:
  static XmlReader FromString(const std::string &data);
  static XmlReader FromFile(const std::string &filename);
  ~XmlReader();

  operator bool() const { return _root != 0; }
  bool operator!() const { return _root == 0; }

  // Close the current file and open a new one
  bool newFile(const std::string &filename);

  bool newString(const std::string &data);

  using Elements = std::vector<TiXmlElement *>;
  static Elements getChildren(const std::string &val, TiXmlElement *elem);
  Elements getChildren(const std::string &val) {
    return getChildren(val, _root);
  }
  static TiXmlElement *findChild(const std::string &val, TiXmlElement *elem);
  TiXmlElement *findChild(const std::string &val) {
    return findChild(val, _root);
  }

  template <typename T>
  static bool findAttr(TiXmlElement *elem, const char *attr, T &val) {
    if (elem == nullptr) return false;

    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal == nullptr) return false;

    std::string strVal(cStrVal);
    std::istringstream iss(strVal);
    iss >> val;
    return true;
  }
  static bool findAttr(TiXmlElement *elem, const char *attr, std::string &val);
  static bool findAttr(TiXmlElement *elem, const char *attr, BasisPoints &val);
#ifndef NO_SDL
  static bool findAttr(TiXmlElement *elem, const char *attr,
                       Color &val);  // Hex string -> Color

  /*
  If a child exists with the specified name, attempt to read its 'mean' and 'sd'
  attributes into the provided variables.  Use default values of mean=0, sd=1.
  mean and sd will be changed if and only if the child is found.
  Return value: whether or not the child was found.
  */
  static bool findNormVarChild(const std::string &name, TiXmlElement *elem,
                               double &mean, double &sd);

  /*
  Find and parse one or more <loot> children.  These can specify either a simple
  chance for an item to drop, or a normal distribution for quantity.
  The results are added to the provided loot table.
  Return value: whether or not the child was found.
  */
  static void findLootChildren(const std::string &name, TiXmlElement *elem,
                               class LootTable &lootTable);

  /*
  If a child exists with the specified name, attempt to read its 'x', 'y', 'w',
  and 'h' attributes into the provided Rect variable.  It will be changed if and
  only if the child is found. Any missing attributes will be assumed to be 0.
  Return value: whether or not the child was found.
  */
  template <typename T>
  static bool findRectChild(const std::string &name, TiXmlElement *elem,
                            Rect<T> &val) {
    TiXmlElement *child = XmlReader::findChild(name, elem);
    if (child == nullptr) return false;
    if (!XmlReader::findAttr(child, "x", val.x)) val.x = 0;
    if (!XmlReader::findAttr(child, "y", val.y)) val.y = 0;
    if (!XmlReader::findAttr(child, "w", val.w)) val.w = 0;
    if (!XmlReader::findAttr(child, "h", val.h)) val.h = 0;
    return true;
  }
#endif  // NO_SDL

  /*
  If a child exists with the specified name, attempt to read its 'armor',
  'maxHealth', etc. attributes into the provided StatsMod variable.  It will be
  changed if and only if the child is found. Any missing attributes will be set
  to their default values. Return value: whether or not the child was found.
  */
  static bool findStatsChild(const std::string &name, TiXmlElement *elem,
                             StatsMod &val);
};

#endif
