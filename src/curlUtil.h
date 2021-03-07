#ifndef CURL_UTIL_H
#define CURL_UTIL_H

#include <string>

std::string readFromURL(const std::string &url,
                        const std::string &userAgent = {});
void downloadFile(const std::string &srcURL, const std::string &dstFilename);

std::string getLocationFromIP(const std::string &ip);

#endif
