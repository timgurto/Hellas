#include "SDLwrappers.h"

#include <SDL.h>

#include "WorkerThread.h"

extern WorkerThread SDLThread;

#define SDL_THREAD_BEGIN(...) SDLThread.callBlocking([__VA_ARGS__](){
#define SDL_THREAD_END \
  });

void SDLWrappers::StopTextInput() {
  SDL_THREAD_BEGIN()
  SDL_StopTextInput();
  SDL_THREAD_END
}
