#include <SDL.h>
#include <cstdlib>
#include <ctime>
#include <string>

#include "../Args.h"
#include "Server.h"

extern "C" {
FILE __iob_func[3] = {*stdin, *stdout, *stderr};
}

Args cmdLineArgs;

int main(int argc, char* argv[]) {
  cmdLineArgs.init(argc, argv);

  srand(static_cast<unsigned>(time(0)));

  Server server;
  server.run();

  return 0;
}
