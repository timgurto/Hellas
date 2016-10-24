// (C) 2015-2016 Tim Gurto

#ifndef XML_READER_H
#define XML_READER_H

#include <set>
#include <string>
#include <tinyxml.h>

class NormalVariable;
struct Rect;

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
    bool newFile(const char *filename);
    bool newFile(const std::string &filename);
    
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

    /*
    If a child exists with the specified name, attempt to read its 'mean' and 'sd' attributes into
    the provided variables.  Use default values of mean=0, sd=1.
    mean and sd will be changed if and only if the child is found.
    Return value: whether or not the child was found.
    */
    static bool findNormVarChild(const std::string &val, TiXmlElement *elem,
                                 double &mean, double &sd);
    /*
    If a child exists with the specified name, attempt to read its 'x', 'y', 'w', and 'h'
    attributes into the provided Rect variable.  It will be changed if and only if the child is
    found.
    Any missing attributes will be assumed to be 0.
    Return value: whether or not the child was found.
    */
    static bool findRectChild(const std::string &val, TiXmlElement *elem, Rect &rect);
};

#endif
