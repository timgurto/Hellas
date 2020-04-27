#include <curl.h>
#include <zip.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../../../src/curlUtil.h"

void extract(const std::string& zipFile) {
  zip* archive;
  int err;
  if (!(archive = zip_open(zipFile.c_str(), 0, &err))) {
    std::cout << "Failed to open archive." << std::endl;
    return;
  }

  auto numFiles = zip_get_num_entries(archive, 0);
  for (int i = 0; i != numFiles; ++i) {
    struct zip_stat stat;
    if (zip_stat_index(archive, i, 0, &stat) != 0) {
      std::cout << "Failed to open file: \"" << stat.name << "\"" << std::endl;
      continue;
    }

    auto progressMsg = std::ostringstream{};
    auto progress = static_cast<int>(1.0 * i / 100 + 0.5);
    progressMsg << "\rExtracting files: " << progress << "%";
    std::cout << progressMsg.str();

    // Create directory if directory
    auto asString = std::string{stat.name};
    auto firstSlash = asString.find('/');
    asString.replace(0, firstSlash, "Hellas client");
    stat.name = asString.c_str();
    CreateDirectory("Hellas client/", NULL);

    if (stat.name[strlen(stat.name) - 1] == '/') {
      if (!CreateDirectory(stat.name, NULL) &&
          GetLastError() != ERROR_ALREADY_EXISTS) {
        auto error = GetLastError();
        std::cout << std::endl
                  << "Failed to create directory " << std::string(stat.name)
                  << std::endl
                  << "Aborting extraction." << std::endl;
        return;
      }
    }

    zip_file* file = zip_fopen_index(archive, i, 0);
    if (!file) {
      std::cout << std::endl
                << "Failed to open file " << std::string(stat.name) << std::endl
                << "Aborting extraction." << std::endl;
      return;
    }

    // int fileDesc = open(stat.name, O_RDWR | O_TRUNC | O_CREAT)
    std::ofstream output(stat.name, std::ofstream::out | std::ofstream::trunc |
                                        std::ofstream::binary);
    auto sum = zip_uint64_t{0};
    static const int BUF_SIZE = 1000000;
    char buffer[BUF_SIZE];
    while (sum != stat.size) {
      auto len = zip_fread(file, buffer, BUF_SIZE);
      if (len < 0) {
        std::cout << std::endl
                  << "Failed to inflate file " << std::string(stat.name)
                  << std::endl
                  << "Aborting extraction." << std::endl;
        return;
      }

      output.write(buffer, len);
      sum += len;
      assert(sum <= stat.size);
    }
    assert(sum == stat.size);
    output.close();
    zip_fclose(file);
  }

  zip_close(archive);
}

int main() {
  std::cout << "Downloading latest client . . ." << std::endl;
  downloadFile("http://hellas.timgurto.com/client.zip", "client.zip");

  extract("client.zip");

  system("cd \"Hellas Client\" && client.exe");
}
