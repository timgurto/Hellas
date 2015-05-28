#include "Log.h"

Log::Log():
_valid(false){}

Log::Log(unsigned displayLines, const std::string &fontName, int fontSize):
_maxMessages(displayLines),
_font(TTF_OpenFont(fontName.c_str(), fontSize)),
_valid(true){
    if (!_font){
        std::string err = TTF_GetError();
        _valid = false;
    }
}

Log::~Log(){
    while (!_messages.empty()){
        SDL_FreeSurface(_messages.front());
        _messages.pop_front();
    }
    if (_font)
        TTF_CloseFont(_font);
}

void Log::operator()(const std::string &message){
    static SDL_Color color = {0xff, 0xff, 0xff, 0};
    SDL_Surface *msgSurface = TTF_RenderText_Solid(_font, message.c_str(), color);
    if (!msgSurface)
        return;
    _messages.push_back(msgSurface);
    if (_messages.size() > _maxMessages){
        SDL_FreeSurface(_messages.front());
        _messages.pop_front();
    }
}

void Log::draw(SDL_Surface *targetSurface, int x, int y) const{
    if (!_valid)
        return;
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    
    for (std::list<SDL_Surface *>::const_iterator it = _messages.begin(); it != _messages.end(); ++it) {
        SDL_Surface *msgSurface = *it;
        SDL_BlitSurface(msgSurface, 0, targetSurface, &rect);
        rect.y += msgSurface->h;
    }

}
