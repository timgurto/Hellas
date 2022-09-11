#include <tinyxml.h>
#include <sstream>

#ifndef XML_WRITER_H
#define XML_WRITER_H

// Wrapper class for TinyXml functionality
class XmlWriter {
  std::string _filename;
  TiXmlDocument _doc;
  TiXmlElement *_root;

 public:
  XmlWriter(const std::string &filename);
  XmlWriter(const char *filename);
  ~XmlWriter();

  void newFile(const std::string &filename);
  void newFile(const char *filename);

  static TiXmlElement *addChild(const char *val, TiXmlElement *elem);
  TiXmlElement *addChild(const char *val) { return addChild(val, _root); }

  template <typename T>
  static void setAttr(TiXmlElement *elem, const char *attr, T val) {
    std::ostringstream oss;
    oss << val;
    elem->SetAttribute(attr, oss.str());
  }
  static void setAttr(TiXmlElement *elem, const char *attr,
                      const std::string &val);
  static void setAttr(TiXmlElement *elem, const char *attr, int val);
  static void setAttr(TiXmlElement *elem, const char *attr, const char *val);
  static void setAttr(TiXmlElement *elem, const char *attr, bool val);

  void publish();
};

#endif
