#include <SDL_ttf.h>
#include <SDL_image.h>

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
        ret = IMG_Init(IMG_INIT_PNG);
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
                               screenX, screenY, screenW, screenH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!_window)
        return;

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!_renderer)
        return;

    SDL_GetRendererOutputSize(_renderer, &_w, &_h);

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

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface) const{
    return SDL_CreateTextureFromSurface(_renderer, surface);
}

SDL_Texture *Renderer::createTargetableTexture(int width, int height) const{
    return SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
}

void Renderer::drawTexture(SDL_Texture *srcTex, const SDL_Rect &dstRect){
    SDL_RenderCopy(_renderer, srcTex, 0, &dstRect);
}

void Renderer::drawTexture(SDL_Texture *srcTex, const SDL_Rect &dstRect, const SDL_Rect &srcRect){
    SDL_RenderCopy(_renderer, srcTex, &srcRect, &dstRect);
}

void Renderer::setDrawColor(const Color &color){
    SDL_SetRenderDrawColor(_renderer, color.r(), color.g(), color.b(), 0xff);
}

void Renderer::clear(){
    SDL_RenderClear(_renderer);
}

void Renderer::present(){
    SDL_RenderPresent(_renderer);
}

void Renderer::drawRect(const SDL_Rect &dstRect){
    SDL_RenderDrawRect(_renderer, &dstRect);
}

void Renderer::fillRect(const SDL_Rect &dstRect){
    SDL_RenderFillRect(_renderer, &dstRect);
}

void Renderer::setRenderTarget() const{
    SDL_SetRenderTarget(_renderer, 0);
}

void Renderer::setScale(float x, float y){
    SDL_RenderSetScale(_renderer, x, y);
}

void Renderer::updateSize(){
    SDL_GetRendererOutputSize(_renderer, &_w, &_h);
}
