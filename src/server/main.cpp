#include <cassert>
#include <ctime>
#include <cstdlib>
#include <string>
#include <SDL.h>

#include "Server.h"
#include "../Args.h"

extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }

Args cmdLineArgs;

int main(int argc, char* argv[]){
    cmdLineArgs.init(argc, argv);

    srand(static_cast<unsigned>(time(0)));

    Server server;
    server.run();

    return 0;
}
