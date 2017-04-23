#include <cassert>

#include <SDL_ttf.h>
#include <SDL_image.h>

#include "Renderer.h"
#include "../Args.h"
#include "../types.h"

extern Args cmdLineArgs;

size_t Renderer::_count = 0;

Renderer::Renderer():
_renderer(nullptr),
_window(nullptr),
_valid(false){
    if (_count == 0) {
        // First renderer constructed; initialize SDL
        int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
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
    const px_t screenX = cmdLineArgs.contains("left") ? cmdLineArgs.getInt("left") :
                                                       SDL_WINDOWPOS_UNDEFINED;
    const px_t screenY = cmdLineArgs.contains("top") ? cmdLineArgs.getInt("top") :
                                                      SDL_WINDOWPOS_UNDEFINED;
    const px_t screenW = cmdLineArgs.contains("width") ? cmdLineArgs.getInt("width") : 1280;
    const px_t screenH = cmdLineArgs.contains("height") ? cmdLineArgs.getInt("height") : 720;

    bool windowedMode = isDebug() || cmdLineArgs.contains("window");
    _window = SDL_CreateWindow("Client", screenX, screenY, screenW, screenH,
                               SDL_WINDOW_SHOWN |( windowedMode ? SDL_WINDOW_RESIZABLE :
                                                                  SDL_WINDOW_FULLSCREEN));
    if (_window == nullptr)
        return;

    _renderer = SDL_CreateRenderer(_window, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (_renderer == nullptr)
        return;

    SDL_GetRendererOutputSize(_renderer, &_w, &_h);

    SDL_RenderSetLogicalSize(_renderer, 640, 360);

    _valid = true;
}

Renderer::~Renderer(){
    if (_renderer != nullptr){
        assert(Texture::numTextures() == 0);
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }
    if (_window != nullptr){
        SDL_DestroyWindow(_window);
        _window = nullptr;
    }
    
    --_count;
    if (_count == 0) {
        TTF_Quit();
        SDL_Quit();
    }
}

SDL_Texture *Renderer::createTextureFromSurface(SDL_Surface *surface) const{
    return SDL_CreateTextureFromSurface(_renderer, surface);
}

SDL_Texture *Renderer::createTargetableTexture(px_t width, px_t height) const{
    return SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                             width, height);
}

void Renderer::drawTexture(SDL_Texture *srcTex, const Rect &dstRect){
    SDL_RenderCopy(_renderer, srcTex, 0, &rectToSDL(dstRect));
}

void Renderer::drawTexture(SDL_Texture *srcTex, const Rect &dstRect, const Rect &srcRect){
    SDL_RenderCopy(_renderer, srcTex, &rectToSDL(srcRect), &rectToSDL(dstRect));
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

void Renderer::drawRect(const Rect &dstRect){
    SDL_RenderDrawRect(_renderer, &rectToSDL(dstRect));
}

void Renderer::fillRect(const Rect &dstRect){
    SDL_RenderFillRect(_renderer, &rectToSDL(dstRect));
}

void Renderer::fill(){
    SDL_RenderFillRect(_renderer, 0);
}

void Renderer::setRenderTarget() const{
    SDL_SetRenderTarget(_renderer, 0);
}

void Renderer::updateSize(){
    SDL_GetRendererOutputSize(_renderer, &_w, &_h);
}

void Renderer::pushRenderTarget(Texture &target) {
    SDL_Texture *currentTarget = SDL_GetRenderTarget(_renderer);
    _renderTargetsStack.push(currentTarget);
    SDL_SetRenderTarget(_renderer, target.raw());
}

void Renderer::popRenderTarget() {
    SDL_SetRenderTarget(_renderer, _renderTargetsStack.top());
    _renderTargetsStack.pop();
}

SDL_Rect Renderer::rectToSDL(const Rect &rect){
    SDL_Rect r = {rect.x, rect.y, rect.w, rect.h};
    return r;
}
