#include "SDLwrappers.h"

#include <SDL.h>

#include "../WorkerThread.h"

#ifdef TESTING
extern WorkerThread SDLThread;
#define SDL_THREAD_BEGIN(...) SDLThread.callBlocking([__VA_ARGS__](){
#define SDL_THREAD_END \
  });
#else
#define SDL_THREAD_BEGIN(...)
#define SDL_THREAD_END
#endif

void SDLWrappers::StopTextInput() {
  SDL_THREAD_BEGIN()
  SDL_StopTextInput();
  SDL_THREAD_END
}

void SDLWrappers::StartTextInput() {
  SDL_THREAD_BEGIN()
  SDL_StartTextInput();
  SDL_THREAD_END
}
