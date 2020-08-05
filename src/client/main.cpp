#include <SDL.h>

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>

#include "../Args.h"
#include "Client.h"
#include "Renderer.h"
#include "Texture.h"

extern "C" {
FILE __iob_func[3] = {*stdin, *stdout, *stderr};
}

Args cmdLineArgs;   // MUST be defined before renderer
Renderer renderer;  // MUST be defined after cmdLineArgs

int main(int argc, char* argv[]) {
  cmdLineArgs.init(argc, argv);
  renderer.init();

  srand(static_cast<unsigned>(time(0)));

  Client client;
  client.run();

  Client::cleanUpStatics();

  return 0;
}
