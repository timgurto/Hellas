#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <cassert>

#include "../Color.h"
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
_raw(nullptr),
_w(0),
_h(0),
_validTarget(false),
_programEndMarker(false){}

Texture::Texture(px_t width, px_t height):
_raw(nullptr),
_w(width),
_h(height),
_validTarget(true),
_programEndMarker(false){
    assert (renderer);

    _raw = renderer.createTargetableTexture(width, height);
    if (_raw != nullptr)
        addRef();
    else
        _validTarget = false;
}

Texture::Texture(const std::string &filename, const Color &colorKey):
_raw(nullptr),
_w(0),
_h(0),
_validTarget(false),
_programEndMarker(false){
    if (filename == "")
        return;
    assert (renderer);

    SDL_Surface *const surface = IMG_Load(filename.c_str());
    if (surface == nullptr)
        return;
    if (&colorKey != &Color::NO_KEY) {
        SDL_SetColorKey(surface, SDL_TRUE, colorKey);
    }
    _raw = renderer.createTextureFromSurface(surface);
    SDL_FreeSurface(surface);
    if (_raw != nullptr)
        addRef();
    const int ret = SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (ret != 0) {
        removeRef();
    _raw = 0;
    }
}

Texture::Texture(SDL_Surface *surface):
_raw(nullptr),
_w(0),
_h(0),
_validTarget(false),
_programEndMarker(false){
    if (surface == nullptr)
        return;
    _raw = renderer.createTextureFromSurface(surface);
    if (_raw != nullptr)
        addRef();
    const int ret = SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (ret != 0) {
        removeRef();
    _raw = 0;
    }
}

Texture::Texture(TTF_Font *font, const std::string &text, const Color &color):
_raw(nullptr),
_w(0),
_h(0),
_validTarget(false),
_programEndMarker(false){
    assert (renderer);

    if (font == nullptr)
        return;

    SDL_Surface *const surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (surface == nullptr)
        return;
    _raw = renderer.createTextureFromSurface(surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (_raw != nullptr)
        addRef();
}

Texture::Texture(const Texture &rhs):
_raw(rhs._raw),
_w(rhs._w),
_h(rhs._h),
_validTarget(rhs._validTarget),
_programEndMarker(false){
    if (_raw != nullptr)
        addRef();
}

Texture &Texture::operator=(const Texture &rhs){
    if (this == &rhs)
        return *this;
    if (_raw != nullptr)
        removeRef();

    _raw = rhs._raw;
    _w = rhs._w;
    _h = rhs._h;
    _validTarget = rhs._validTarget;
    if (_raw != nullptr)
        addRef();
    return *this;
}

Texture::~Texture(){
    if (_programEndMarker) {
        /*
        Program has ended; only static Textures are being destroyed now.
        Destroy all SDL_Textures before _refs is destroyed, then clear _refs.
        */
        for (const std::pair<SDL_Texture *, size_t> &ref : _refs)
            SDL_DestroyTexture(ref.first);
        _refs.clear();
        return;
    }

    if (_raw != nullptr)
        removeRef();
}

void Texture::setBlend(SDL_BlendMode mode) const{
    SDL_SetTextureBlendMode(_raw, mode);
}

void Texture::setAlpha(Uint8 alpha) const{
    SDL_SetTextureAlphaMod(_raw, alpha);
}

void Texture::draw(px_t x, px_t y) const{
    draw(Rect(x, y, _w, _h));
}

void Texture::draw(const Point &location) const{
    draw(toInt(location.x), toInt(location.y));
}

void Texture::draw(const Rect &location) const{
    renderer.drawTexture(_raw, location);
}

void Texture::draw(const Rect &location, const Rect &srcRect) const{
    renderer.drawTexture(_raw, location, srcRect);
}

void Texture::addRef(){
    ++_numTextures;
    const std::map<SDL_Texture *, size_t>::iterator it = _refs.find(_raw);
    if (it != _refs.end())
        ++ it->second;
    else
        _refs[_raw] = 1;
}

void Texture::removeRef(){
    if (_raw != nullptr && !_refs.empty()) {
        --_numTextures;
        --_refs[_raw];
        if (_refs[_raw] == 0) {
            SDL_DestroyTexture(_raw);
            _refs.erase(_raw);
        }
    }
}

void Texture::setRenderTarget() const{
    if (_validTarget)
        SDL_SetRenderTarget(renderer._renderer, _raw);
}
