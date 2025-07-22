#ifndef CURL_UTIL_H
#define CURL_UTIL_H

#include <string>

std::string readFromURL(const std::string &url,
                        const std::string &userAgent = "curl/7.88.1");
void downloadFile(const std::string &srcURL, const std::string &dstFilename);

std::string getLocationFromIP(const std::string &ip);

#endif
