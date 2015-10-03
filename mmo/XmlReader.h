// (C) 2015 Tim Gurto

#ifndef XML_READER_H
#define XML_READER_H

#include <string>
#include <tinyxml.h>

#include "Log.h"

// Wrapper class for TinyXml functionality
class XmlReader{
    Log *_debug;
    TiXmlDocument _doc;
    TiXmlElement *_root;

public:
    XmlReader(const char *filename, Log *debug = 0);
    ~XmlReader();

    operator bool() const { return _root != 0; }
    bool operator!() const { return _root == 0; }

    void newFile(const char *filename); // Close the current file and open a new one
    
    static std::set<TiXmlElement *> getChildren(const std::string &val, TiXmlElement *elem);
    std::set<TiXmlElement *> getChildren(const std::string &val) { return getChildren(val, _root); }

    template<typename T>
    static bool findAttr(TiXmlElement *elem, const char *attr, T &val) {
        const char *const cStrVal = elem->Attribute(attr);
        if (cStrVal) {
            std::string strVal(cStrVal);
            std::istringstream iss(strVal);
            iss >> val;
            return true;
        }
        return false;
    }
    static bool findAttr(TiXmlElement *elem, const char *attr, std::string &val);
};

#endif
