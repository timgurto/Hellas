#include <set>
#include <sstream>

#include "XmlReader.h"

#ifndef NO_SDL
#include "NormalVariable.h"
#include "Rect.h"
#endif // NO_SDL

XmlReader::XmlReader(const char *filename):
_root(nullptr){
    newFile(filename);
}

XmlReader::XmlReader(const std::string &filename):
_root(nullptr){
    newFile(filename);
}

XmlReader::~XmlReader(){
    _doc.Clear();
}

bool XmlReader::newFile(const char *filename){
    _doc.Clear();
    _root = nullptr;
    if (!_doc.LoadFile(filename))
        return false;
    _root = _doc.FirstChildElement();
    return *this;
}

bool XmlReader::newFile(const std::string &filename){
    return newFile(filename.c_str());
}

std::set<TiXmlElement *> XmlReader::getChildren(const std::string &val, TiXmlElement *elem) {
    std::set<TiXmlElement *> children;
    if (elem == nullptr)
        return children;
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.insert(child);
    return children;
}

TiXmlElement *XmlReader::findChild(const std::string &val, TiXmlElement *elem){
    if (elem == nullptr)
        return false;
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

bool XmlReader::findRectChild(const std::string &val, TiXmlElement *elem, Rect &rect){
    TiXmlElement *child = XmlReader::findChild(val, elem);
    if (child == nullptr)
        return false;
    if (!XmlReader::findAttr(child, "x", rect.x)) rect.x = 0;
    if (!XmlReader::findAttr(child, "y", rect.y)) rect.y = 0;
    if (!XmlReader::findAttr(child, "w", rect.w)) rect.w = 0;
    if (!XmlReader::findAttr(child, "h", rect.h)) rect.h = 0;
    return true;
}
#endif // NO_SDL
