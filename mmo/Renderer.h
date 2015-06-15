#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>

// Wrapper class for SDL_Renderer and SDL_Window, plus related convenience functions.
class Renderer{
    SDL_Renderer *_renderer;
    SDL_Window *_window;
    bool _valid; // Whether everything has been properly initialized
    static size_t _count;

public:
    Renderer();
    ~Renderer();

    /*
    Some construction takes place here, to be called manually, as a Renderer may be static.
    Specifically, this creates the window and renderer objects, and requires that cmdLineArgs has been populated.
    */
    void init();

    operator SDL_Renderer* (); //TODO: Remove
    operator bool() const;
};

#endif
