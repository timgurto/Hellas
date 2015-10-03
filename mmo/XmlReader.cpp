// (C) 2015 Tim Gurto

#include <set>
#include <sstream>

#include "XmlReader.h"

XmlReader::XmlReader(const char *filename):
_root(0){
    newFile(filename);
}

XmlReader::~XmlReader(){
    _doc.Clear();
}

void XmlReader::newFile(const char *filename){
    _doc.Clear();
    _root = 0;
    bool ret = _doc.LoadFile(filename);
    if (!ret)
        return;
    _root = _doc.FirstChildElement();
}

std::set<TiXmlElement *> XmlReader::getChildren(const std::string &val, TiXmlElement *elem) {
    std::set<TiXmlElement *> children;
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.insert(child);
    return children;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, std::string &val){
    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal) {
        val = cStrVal;
        return true;
    }
    return false;
}
