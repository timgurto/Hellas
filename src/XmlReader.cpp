#include <set>
#include <sstream>

#include "Stats.h"
#include "XmlReader.h"

#ifndef NO_SDL
#include "NormalVariable.h"
#include "Rect.h"
#endif // NO_SDL

XmlReader::XmlReader(const std::string &string, bool isFile) :
_root(nullptr) {
        newFile(string);
    }
}

XmlReader XmlReader::FromString(const std::string & data) {
    return XmlReader(data, false);
}

XmlReader XmlReader::FromFile(const std::string & filename) {
    return XmlReader(filename, true);
}

XmlReader::~XmlReader(){
    _doc.Clear();
}

    _doc.Clear();
    _root = nullptr;
    if (!_doc.LoadFile(filename))
        return false;
    _root = _doc.FirstChildElement();
    return *this;
}

bool XmlReader::newFile(const std::string &filename) {
    _doc.Clear();
    _root = nullptr;
    if (!_doc.LoadFile(filename))
        return false;
    _root = _doc.FirstChildElement();
    return *this;
}

XmlReader::Elements XmlReader::getChildren(const std::string &val, TiXmlElement *elem) {
    if (elem == nullptr)
        return{};

    auto children = Elements{};
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.push_back(child);
    return children;
}

TiXmlElement *XmlReader::findChild(const std::string &val, TiXmlElement *elem){
    if (elem == nullptr)
        return nullptr;

    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            return child;
    return nullptr;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, std::string &val){
    if (elem == nullptr)
        return false;

    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal != nullptr) {
        val = cStrVal;
        return true;
    }
    return false;
}

#ifndef NO_SDL
bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, Color &val){
    if (elem == nullptr)
        return false;

    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal != nullptr){
        std::istringstream iss(cStrVal);
        Uint32 color;
        iss >> std::hex >> color;
        Uint8
            b = color % 0x100,
            g = color >> 8 % 0x100,
            r = color >> 16 % 100;
        val = Color(r, g, b);
        return true;
    }
    return false;
}

bool XmlReader::findNormVarChild(const std::string &val, TiXmlElement *elem,
                                        double &mean, double &sd){
    TiXmlElement *child = XmlReader::findChild(val, elem);
    if (child == nullptr)
        return false;
    if (!XmlReader::findAttr(child, "mean", mean)) mean = 0;
    if (!XmlReader::findAttr(child, "sd", sd)) sd = 1;
    return true;
}
#endif // NO_SDL

bool XmlReader::findStatsChild(const std::string & val, TiXmlElement * elem, StatsMod & stats) {
    TiXmlElement *child = XmlReader::findChild(val, elem);
    if (child == nullptr)
        return false;

    stats = StatsMod{};
    XmlReader::findAttr(child, "armor", stats.armor);
    XmlReader::findAttr(child, "health", stats.maxHealth);
    XmlReader::findAttr(child, "energy", stats.maxEnergy);
    XmlReader::findAttr(child, "hps", stats.hps);
    XmlReader::findAttr(child, "eps", stats.eps);
    XmlReader::findAttr(child, "hit", stats.hit);
    XmlReader::findAttr(child, "crit", stats.crit);
    XmlReader::findAttr(child, "critResist", stats.critResist);
    XmlReader::findAttr(child, "dodge", stats.dodge);
    XmlReader::findAttr(child, "block", stats.block);
    XmlReader::findAttr(child, "blockValue", stats.blockValue);
    XmlReader::findAttr(child, "magicDamage", stats.magicDamage);
    XmlReader::findAttr(child, "physicalDamage", stats.physicalDamage);
    XmlReader::findAttr(child, "healing", stats.healing);
    XmlReader::findAttr(child, "airResist", stats.airResist);
    XmlReader::findAttr(child, "earthResist", stats.earthResist);
    XmlReader::findAttr(child, "fireResist", stats.fireResist);
    XmlReader::findAttr(child, "waterResist", stats.waterResist);
    XmlReader::findAttr(child, "attackTime", stats.attackTime);
    XmlReader::findAttr(child, "speed", stats.speed);
    XmlReader::findAttr(child, "gatherBonus", stats.gatherBonus);

    auto n = 0;
    stats.stuns = XmlReader::findAttr(child, "stuns", n) && n != 0;

    return true;
}
