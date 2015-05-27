#include "Log.h"

Log::Log():
_valid(false){}

Log::Log(unsigned displayLines, const std::string &fontName, int fontSize):
_maxMessages(displayLines),
_font(TTF_OpenFont(fontName.c_str(), fontSize)),
_valid(true),
_lock(SDL_CreateSemaphore(1)){
    if (!_font){
        std::string err = TTF_GetError();
        _valid = false;
    }
}

Log::~Log(){
    if (_font)
        TTF_CloseFont(_font);
    SDL_DestroySemaphore(_lock);
}

void Log::operator()(const std::string &message){
    SDL_SemWait(_lock);
    _messages.push_back(message);
    if (_messages.size() > _maxMessages)
        _messages.pop_front();
    SDL_SemPost(_lock);
}

void Log::draw(SDL_Surface *targetSurface, int x, int y) {
    if (!_valid)
        return;
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    static SDL_Color color = {0xff, 0xff, 0xff, 0};
    
    SDL_SemWait(_lock);
    for (std::list<std::string>::const_iterator it = _messages.begin(); it != _messages.end(); ++it) {
        SDL_Surface* msg = TTF_RenderText_Solid( _font, it->c_str(), color );
        if (!msg){
            std::string err = TTF_GetError();
            continue;
        }
        SDL_BlitSurface(msg, 0, targetSurface, &rect);
        rect.y += msg->h;
        SDL_FreeSurface(msg);
    }
    SDL_SemPost(_lock);

}
