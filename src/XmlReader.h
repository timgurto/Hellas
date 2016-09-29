// (C) 2015-2016 Tim Gurto

#ifndef XML_READER_H
#define XML_READER_H

#include <string>
#include <tinyxml.h>

// Wrapper class for TinyXml functionality
class XmlReader{
    TiXmlDocument _doc;
    TiXmlElement *_root;

public:
    XmlReader(const char *filename);
    XmlReader(const std::string &filename);
    ~XmlReader();

    operator bool() const { return _root != 0; }
    bool operator!() const { return _root == 0; }
    
    // Close the current file and open a new one
    void newFile(const char *filename);
    void newFile(const std::string &filename);
    
    static std::set<TiXmlElement *> getChildren(const std::string &val, TiXmlElement *elem);
    std::set<TiXmlElement *> getChildren(const std::string &val) { return getChildren(val, _root); }
    static TiXmlElement *findChild(const std::string &val, TiXmlElement *elem);
    TiXmlElement *findChild(const std::string &val) { return findChild(val, _root); }

    template<typename T>
    static bool findAttr(TiXmlElement *elem, const char *attr, T &val) {
        const char *const cStrVal = elem->Attribute(attr);
        if (cStrVal != nullptr) {
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
