#include <cassert>

#include "../Color.h"
#include "Renderer.h"
#include "Surface.h"
#include "Texture.h"

extern Renderer renderer;

std::map<SDL_Texture *, size_t> Texture::_refs; // MUST be defined before _programEndMarkerTexture 
/*
MUST be defined after _refs, so that it is destroyed first.
This object's d'tor signals that the program has ended.
*/
Texture Texture::_programEndMarkerTexture(true);

std::map<const SDL_Texture*, std::string> Texture::_descriptions;

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

    if (isDebug()){
        std::ostringstream oss;
        oss << "Blank " << width << 'x' << height;
        _descriptions[_raw] = oss.str();
    }
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

    Surface surface(filename, colorKey);
    if (!surface)
        return;
    _raw = surface.toTexture();
    if (_raw != nullptr)
        addRef();
    const int ret = SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (ret != 0) {
        removeRef();
        _raw = 0;
    }

    if (isDebug()){
        _descriptions[_raw] = filename;
    }
}

Texture::Texture(const Surface &surface):
_raw(nullptr),
_w(0),
_h(0),
_validTarget(false),
_programEndMarker(false){
    if (!surface)
        return;
    _raw = surface.toTexture();
    if (_raw != nullptr)
        addRef();
    const int ret = SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (ret != 0) {
        removeRef();
        _raw = 0;
    }

    if (isDebug()){
        _descriptions[_raw] = "From surface: " + surface.description();
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

    Surface surface(font, text, color);
    if (!surface)
        return;
    _raw = surface.toTexture();
    if (_raw != nullptr)
        addRef();
    const int ret = SDL_QueryTexture(_raw, nullptr, nullptr, &_w, &_h);
    if (ret != 0) {
        removeRef();
        _raw = 0;
    }

    if (isDebug()){
        _descriptions[_raw] = "Text: " + text;
    }
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
        destroyAllRemainingTextures();
        return;
    }

    if (_raw != nullptr)
        removeRef();
}

void Texture::destroyAllRemainingTextures(){
    for (const std::pair<SDL_Texture *, size_t> &ref : _refs)
        SDL_DestroyTexture(ref.first);
    _refs.clear();
}


void Texture::setBlend(SDL_BlendMode mode) const{
    SDL_SetTextureBlendMode(_raw, mode);
}

void Texture::setAlpha(Uint8 alpha) const{
    SDL_SetTextureAlphaMod(_raw, alpha);
}

void Texture::draw(px_t x, px_t y) const{
    draw({ x, y, _w, _h });
}

void Texture::draw(const ScreenPoint &location) const{
    draw(toInt(location.x), toInt(location.y));
}

void Texture::draw(const ScreenRect &location) const{
    renderer.drawTexture(_raw, location);
}

void Texture::draw(const ScreenRect &location, const ScreenRect &srcRect) const{
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
            if (isDebug())
                _descriptions.erase(_raw);
            SDL_DestroyTexture(_raw);
            _refs.erase(_raw);
        }
    }
}

void Texture::setRenderTarget() const{
    if (_validTarget)
        SDL_SetRenderTarget(renderer._renderer, _raw);
}
