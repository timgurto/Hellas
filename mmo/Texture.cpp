// (C) 2015 Tim Gurto

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <cassert>

#include "Color.h"
#include "Renderer.h"
#include "Texture.h"

extern Renderer renderer;

std::map<SDL_Texture *, size_t> Texture::_refs; // MUST be defined before _programEndMarkerTexture 
/*
MUST be defined after _refs, so that it is destroyed first.
This object's d'tor signals that the program has ended.
*/
Texture Texture::_programEndMarkerTexture(true); 

int Texture::_numTextures = 0;

Texture::Texture():
_programEndMarker(false),
_raw(0),
_w(0),
_h(0),
_validTarget(false){}

Texture::Texture(int width, int height):
_programEndMarker(false),
_raw(0),
_w(width),
_h(height),
_validTarget(true){
    assert (renderer);

    _raw = renderer.createTargetableTexture(width, height);
    if (_raw)
        addRef();
    else
        _validTarget = false;
}

Texture::Texture(const std::string &filename, const Color &colorKey):
_programEndMarker(false),
_w(0),
_h(0),
_validTarget(false),
_raw(0){
    if (filename == "")
        return;
    assert (renderer);

    SDL_Surface *surface = IMG_Load(filename.c_str());
    if (!surface)
        return;
    if (&colorKey != &Color::NO_KEY) {
        SDL_SetColorKey(surface, SDL_TRUE, colorKey);
    }
    _raw = renderer.createTextureFromSurface(surface);
    SDL_FreeSurface(surface);
    int ret = SDL_QueryTexture(_raw, 0, 0, &_w, &_h);
    if (_raw)
        addRef();
}

Texture::Texture(TTF_Font *font, const std::string &text, const Color &color):
_programEndMarker(false),
_w(0),
_h(0),
_validTarget(false),
_raw(0){
    assert (renderer);

    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface)
        return;
    _raw = renderer.createTextureFromSurface(surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(_raw, 0, 0, &_w, &_h);
    if (_raw)
        addRef();
}

Texture::Texture(const Texture &rhs):
_programEndMarker(false),
_raw(rhs._raw),
_w(rhs._w),
_h(rhs._h),
_validTarget(rhs._validTarget){
    if (_raw)
        addRef();
}

Texture &Texture::operator=(const Texture &rhs){
    if (this == &rhs)
        return *this;
    if (_raw)
        removeRef();

    _raw = rhs._raw;
    _w = rhs._w;
    _h = rhs._h;
    _validTarget = rhs._validTarget;
    if (_raw)
        addRef();
    return *this;
}

Texture::~Texture(){
    if (_programEndMarker) {
        /*
        Program has ended; only static Textures are being destroyed now.
        Destroy all SDL_Textures before _refs is destroyed, then clear _refs.
        */
        for (std::map<SDL_Texture *, size_t>::iterator it = _refs.begin(); it != _refs.end(); ++it) {
            SDL_DestroyTexture(it->first);
        }
        _refs.clear();
        return;
    }

    if (_raw)
        removeRef();
}

void Texture::setBlend(SDL_BlendMode mode) const{
    SDL_SetTextureBlendMode(_raw, mode);
}

void Texture::setAlpha(Uint8 alpha) const{
    SDL_SetTextureAlphaMod(_raw, alpha);
}

void Texture::draw(int x, int y) const{
    SDL_Rect r = {x, y, _w, _h};
    draw(r);
}

void Texture::draw(const Point &location) const{
    draw(static_cast<int>(location.x + .5), static_cast<int>(location.y + .5));
}

void Texture::draw(const SDL_Rect &location) const{
    renderer.drawTexture(_raw, location);
}

void Texture::draw(const SDL_Rect &location, const SDL_Rect &srcRect) const{
    renderer.drawTexture(_raw, location, srcRect);
}

void Texture::addRef(){
    ++_numTextures;
    std::map<SDL_Texture *, size_t>::iterator it = _refs.find(_raw);
    if (it != _refs.end())
        ++ it->second;
    else
        _refs[_raw] = 1;
}

void Texture::removeRef(){
    if (_raw && !_refs.empty()) {
        --_numTextures;
        --_refs[_raw];
        if (_refs[_raw] == 0) {
            SDL_DestroyTexture(_raw);
            int ret = _refs.erase(_raw);
        }
    }
}

void Texture::setRenderTarget() const{
    if (_validTarget)
        SDL_SetRenderTarget(renderer._renderer, _raw);
}
