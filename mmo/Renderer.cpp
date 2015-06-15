#include <SDL_ttf.h>

#include "Args.h"
#include "Renderer.h"

extern Args cmdLineArgs;

size_t Renderer::_count = 0;

Renderer::Renderer():
_valid(false),
_window(0),
_renderer(0){
    if (_count == 0) {
        // First renderer constructed; initialize SDL
        int ret = SDL_Init(SDL_INIT_VIDEO);
        if (ret < 0)
            return;
        ret = TTF_Init();
        if (ret < 0)
            return;
    }
    ++_count;
}

void Renderer::init(){
    int screenX = cmdLineArgs.contains("left") ?
                  cmdLineArgs.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = cmdLineArgs.contains("top") ?
                  cmdLineArgs.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenW = cmdLineArgs.contains("width") ?
                  cmdLineArgs.getInt("width") :
                  640;
    int screenH = cmdLineArgs.contains("height") ?
                  cmdLineArgs.getInt("height") :
                  480;

    _window = SDL_CreateWindow((cmdLineArgs.contains("server") ? "Server" : "Client"),
                               screenX, screenY, screenW, screenH, SDL_WINDOW_SHOWN);
    if (!_window)
        return;

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    if (!_renderer)
        return;

    _valid = true;
}

Renderer::~Renderer(){
    if (_renderer)
        SDL_DestroyRenderer(_renderer);
    if (_window)
        SDL_DestroyWindow(_window);
    
    --_count;
    if (_count == 0) {
        TTF_Quit();
        SDL_Quit();
    }
}

Renderer::operator SDL_Renderer* (){
    return _renderer;
}

Renderer::operator bool() const{
    return _valid;
}
