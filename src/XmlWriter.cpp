#include "XmlWriter.h"

XmlWriter::XmlWriter(const std::string &filename) : _filename(filename) {
  _root = new TiXmlElement("root");
  _doc.LinkEndChild(_root);
}

XmlWriter::XmlWriter(const char *filename) : _filename(filename) {
  _root = new TiXmlElement("root");
  _doc.LinkEndChild(_root);
}

XmlWriter::~XmlWriter() { _doc.Clear(); }

void XmlWriter::newFile(const std::string &filename) {
  _doc.Clear();
  _filename = filename;
  _root = new TiXmlElement("root");
  _doc.LinkEndChild(_root);
}

void XmlWriter::newFile(const char *filename) {
  newFile(std::string(filename));
}

TiXmlElement *XmlWriter::addChild(const char *val, TiXmlElement *elem) {
  TiXmlElement *e = new TiXmlElement(val);
  elem->LinkEndChild(e);
  return e;
}

void XmlWriter::setAttr(TiXmlElement *elem, const char *attr,
                        const std::string &val) {
  elem->SetAttribute(attr, val);
}

void XmlWriter::setAttr(TiXmlElement *elem, const char *attr, int val) {
  elem->SetAttribute(attr, val);
}

void XmlWriter::setAttr(TiXmlElement *elem, const char *attr, const char *val) {
  elem->SetAttribute(attr, val);
}

void XmlWriter::setAttr(TiXmlElement *elem, const char *attr, bool val) {
  elem->SetAttribute(attr, val ? 1 : 0);
}

void XmlWriter::publish() { _doc.SaveFile(_filename); }
