// (C) 2015 Tim Gurto

#include <string>
#include "tinyxml.h"

bool findStrAttr(TiXmlElement *elem, const char *attr, std::string &val);

bool findIntAttr(TiXmlElement *elem, const char *attr, int &val);
