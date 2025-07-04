#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>

#include "../Args.h"
#include "../WorkerThread.h"
#include "Client.h"
#include "Options.h"
#include "Renderer.h"
#include "Texture.h"

extern "C" {
FILE __iob_func[3] = {*stdin, *stdout, *stderr};
}

Args cmdLineArgs;  // MUST be defined before renderer
Options options;
Renderer renderer;

int main(int argc, char* argv[]) {
  cmdLineArgs.init(argc, argv);
  options.load();
  renderer.init();

  srand(static_cast<unsigned>(time(0)));

  Client client;
  client.run();

  Client::cleanUpStatics();

  return 0;
}
