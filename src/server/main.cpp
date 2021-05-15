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

static void disableWindowsCrashDialog() {
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
}

int main(int argc, char* argv[]) {
  disableWindowsCrashDialog();

  cmdLineArgs.init(argc, argv);

  srand(static_cast<unsigned>(time(0)));

  Server server;
  server.run();

  return 0;
}
