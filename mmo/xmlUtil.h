// (C) 2015 Tim Gurto

#include <string>
#include "tinyxml.h"

#include "Log.h"

TiXmlElement *openAndGetRoot(const char *filename, TiXmlDocument &doc, Log *debug = 0);

bool findStrAttr(TiXmlElement *elem, const char *attr, std::string &val);
bool findIntAttr(TiXmlElement *elem, const char *attr, int &val);
