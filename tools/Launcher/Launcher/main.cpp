#include <curl.h>

#include "../../../src/curlUtil.h"

int main() {
  downloadFile("http://hellas.timgurto.com/client.zip", "client.zip");
}
