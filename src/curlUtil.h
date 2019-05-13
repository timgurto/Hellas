#ifndef CURL_UTIL_H
#define CURL_UTIL_H

#include <string>

std::string readFromURL(const std::string &url);

std::string getLocationFromIP(const std::string &ip);

#endif
