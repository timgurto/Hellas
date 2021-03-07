#include "curlUtil.h"

#include <curl.h>

static size_t writeMemoryCallback(void *ptr, size_t size, size_t nmemb,
                                  void *data) {
  size_t realSize = size * nmemb;
  std::string *output = reinterpret_cast<std::string *>(data);
  const char *cp = reinterpret_cast<const char *>(ptr);
  *output = std::string(cp, realSize);
  return 0;
}

std::string readFromURL(const std::string &url, const std::string &userAgent) {
  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    curl_global_cleanup();
    return {};
  }
  std::string output;
  if (!userAgent.empty()) {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&output));
  curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return output;
}

void downloadFile(const std::string &srcURL, const std::string &dstFilename) {
  auto curl = curl_easy_init();
  if (!curl) {
    return;
  }

  FILE *outFile;
  auto err = fopen_s(&outFile, dstFilename.c_str(), "wb");
  if (err) {
    curl_global_cleanup();
    return;
  }

  curl_easy_setopt(curl, CURLOPT_URL, srcURL.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, outFile);
  curl_easy_perform(curl);
  fclose(outFile);

  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

std::string getLocationFromIP(const std::string &ip) {
  auto result = readFromURL("https://tools.keycdn.com/geo.json?host=" + ip,
                            "keycdn-tools:http://playhellas.com");
  return result;
}
