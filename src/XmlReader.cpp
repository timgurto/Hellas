// (C) 2015-2016 Tim Gurto

#include <set>
#include <sstream>

#include "NormalVariable.h"
#include "Rect.h"
#include "XmlReader.h"

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
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.insert(child);
    return children;
}

TiXmlElement *XmlReader::findChild(const std::string &val, TiXmlElement *elem){
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            return child;
    return nullptr;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, std::string &val){
    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal != nullptr) {
        val = cStrVal;
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
