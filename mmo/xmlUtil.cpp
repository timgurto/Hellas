// (C) 2015 Tim Gurto

#include <sstream>

#include "xmlUtil.h"

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
