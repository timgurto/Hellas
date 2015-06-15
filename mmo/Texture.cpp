#include <SDL.h>
#include <SDL_ttf.h>
#include <cassert>

#include "Args.h"
#include "Color.h"
#include "Texture.h"

extern Args cmdLineArgs;


SDL_Window *Texture::_window = 0;
SDL_Renderer *Texture::renderer = 0;
bool Texture::_initialized = false;
std::map<SDL_Texture *, size_t> Texture::_refs; // MUST be defined before _programEndMarkerTexture 
Texture Texture::_programEndMarkerTexture(true); // MUST be defined after _refs
int Texture::_numTextures = 0;

Texture::Texture():
_programEndMarker(false),
_raw(0),
_w(0),
_h(0){
}

Texture::Texture(const std::string &filename, const Color &colorKey):
_programEndMarker(false),
_w(0),
_h(0),
_raw(0){
    if (filename == "")
        return;

    if (!_initialized) initRenderer();

    SDL_Surface *surface = SDL_LoadBMP(filename.c_str());
    if (!surface)
        return;
    if (&colorKey != &Color::NO_KEY) {
        SDL_SetColorKey(surface, SDL_TRUE, colorKey);
    }
    _raw = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    int ret = SDL_QueryTexture(_raw, 0, 0, &_w, &_h);
    if (_raw)
        addRef();
}

Texture::Texture(TTF_Font *font, const std::string &text, const Color &color):
_programEndMarker(false),
_w(0),
_h(0),
_raw(0){
    if (!_initialized) initRenderer();

    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface)
        return;
    _raw = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(_raw, 0, 0, &_w, &_h);
    if (_raw)
        addRef();
}

Texture::Texture(const Texture &rhs):
_programEndMarker(false),
_raw(rhs._raw),
_w(rhs._w),
_h(rhs._h){
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
        destroyRenderer();
        return;
    }

    if (_raw)
        removeRef();
}

bool Texture::operator!() const{
    return _raw == 0;
}

Texture::operator bool() const{
    return _raw != 0;
}

int Texture::width() const{
    return _w;
}

int Texture::height() const{
    return _h;
}

void Texture::draw(int x, int y) const{
    SDL_Rect r = {x, y, _w, _h};
    draw(r);
}

void Texture::draw(const Point &location) const{
    draw(static_cast<int>(location.x + .5), static_cast<int>(location.y + .5));
}

void Texture::draw(const SDL_Rect &location) const{
    SDL_RenderCopy(renderer, _raw, 0, &location);
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

int Texture::numTextures(){
    return _numTextures;
}

void Texture::initRenderer(){
    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return;
    ret = TTF_Init();
    if (ret < 0)
        return;

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

    renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        return;

    _initialized = true;
}

void Texture::destroyRenderer(){
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (_window)
        SDL_DestroyWindow(_window);
    
    TTF_Quit();
    SDL_Quit();
}
