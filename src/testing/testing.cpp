#define CATCH_CONFIG_RUNNER
#include <SDL.h>

#include "../Args.h"
#include "../WorkerThread.h"
#include "../client/Client.h"
#include "../client/Options.h"
#include "../client/Renderer.h"
#include "catch.hpp"

extern "C" {
FILE __iob_func[3] = {*stdin, *stdout, *stderr};
}

Args cmdLineArgs;  // MUST be defined before renderer
Options options;

// SDL requires all calls to be in the same thread.
// Because the test project spins out clients into separate threads, there is
// additional infrastructure to ensure the one thread is used.
// The normal Client project does not have these protections (for performance
// reasons), and thus assumes that all SDL calls occur in the main thread.
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
