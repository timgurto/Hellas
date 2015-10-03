// (C) 2015 Tim Gurto

#include <sstream>
#include <tinyxml.h>

#ifndef XML_WRITER_H
#define XML_WRITER_H

// Wrapper class for TinyXml functionality
class XmlWriter{
    std::string _filename;
    TiXmlDocument _doc;
    TiXmlElement *_root;

public:
    XmlWriter(const std::string &filename);
    ~XmlWriter();

    static TiXmlElement *addChild(const char *val, TiXmlElement *elem);
    TiXmlElement *addChild(const char *val) { return addChild(val, _root); }

    template<typename T>
    static void setAttr(TiXmlElement *elem, const char *attr, T val){
        std::ostringstream oss;
        oss << val;
        elem->SetAttribute(attr, oss.str());
    }
    static void setAttr(TiXmlElement *elem, const char *attr, const std::string &val);
    static void setAttr(TiXmlElement *elem, const char *attr, int val);
    static void setAttr(TiXmlElement *elem, const char *attr, const char *val);

    void publish();
};

#endif
