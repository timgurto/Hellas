// (C) 2016 Tim Gurto

#include <curl.h>

#include "curlUtil.h"

size_t writeMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){
    size_t realSize = size * nmemb;
    std::string * output = reinterpret_cast<std::string *>(data);
    const char *cp = reinterpret_cast<const char *>(ptr);
    *output = std::string(cp, realSize);
    return 0;
}

std::string readFromURL(const std::string &url){
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if (curl == nullptr){
        curl_global_cleanup();
        return "";
    }
    std::string output;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&output));
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return output;
}
