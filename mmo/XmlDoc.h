// (C) 2015 Tim Gurto

#ifndef XML_DOC_H
#define XML_DOC_H

#include <string>
#include "tinyxml.h"

#include "Log.h"

// Wrapper class for TinyXml functionality
class XmlDoc{
    Log *_debug;
    TiXmlDocument _doc;
    TiXmlElement *_root;

public:
    XmlDoc(const char *filename, Log *debug = 0);
    ~XmlDoc();
    
    static std::set<TiXmlElement *> getChildren(const std::string &val, TiXmlElement *elem);
    std::set<TiXmlElement *> getChildren(const std::string &val) { return getChildren(val, _root); }

    static bool findStrAttr(TiXmlElement *elem, const char *attr, std::string &val);
    static bool findIntAttr(TiXmlElement *elem, const char *attr, int &val);
};

#endif