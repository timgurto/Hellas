#define CATCH_CONFIG_RUNNER
#include <SDL.h>

#include "../Args.h"
#include "../client/Client.h"
#include "../client/Renderer.h"
#include "../client/WorkerThread.h"
#include "catch.hpp"

extern "C" {
FILE __iob_func[3] = {*stdin, *stdout, *stderr};
}

Args cmdLineArgs;  // MUST be defined before renderer

// Because the test project spins out clients into separate threads, there is
// additional infrastructure to ensure all SDL calls happen within the one
// thread.
WorkerThread SDLThread{"SDL worker"};  // MUST be defined before renderer

Renderer renderer;

int main(int argc, char* argv[]) {
  srand(static_cast<unsigned>(time(0)));

  cmdLineArgs.init(argc, argv);
  cmdLineArgs.add("new");
  cmdLineArgs.add("server-ip", "127.0.0.1");
  cmdLineArgs.add("window");
  cmdLineArgs.add("quiet");
  cmdLineArgs.add("user-files-path", "testing/users");
  cmdLineArgs.add("hideLoadingScreen");
  cmdLineArgs.add("debug");

  renderer.init();

  auto result = Catch::Session().run(argc, argv);

  Client::cleanUpStatics();

  return (result < 0xff ? result : 0xff);
}
