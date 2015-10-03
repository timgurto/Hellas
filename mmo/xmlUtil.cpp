// (C) 2015 Tim Gurto

#include <sstream>

#include "xmlUtil.h"

TiXmlElement *openAndGetRoot(const char *filename, TiXmlDocument &doc, Log *debug){
    bool ret = doc.LoadFile(filename);
    if (!ret) {
        if (debug)
            *debug << Color::RED << "Failed to load XML file " << filename << ": "
                   << doc.ErrorDesc() << Log::endl;
        return 0;
    }
    TiXmlElement *const root = doc.FirstChildElement();
    if (!root && debug)
        *debug << Color::RED
               << "XML file " << filename << "has no root node; aborting." << Log::endl;
    return root;
}

bool findStrAttr(TiXmlElement *elem, const char *attr, std::string &strVal){
    const char *const val = elem->Attribute(attr);
    if (val) {
        strVal = val;
        return true;
    }
    return false;
}

bool findIntAttr(TiXmlElement *elem, const char *attr, int &intVal){
    const char *const val = elem->Attribute(attr);
    if (val) {
        std::string strVal(val);
        std::istringstream iss(strVal);
        iss >> intVal;
        return true;
    }
    return false;
}
