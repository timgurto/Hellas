#include "Client.h"
#include "Server.h"
#include "Log.h"

Log::Log():
_valid(false){}

Log::Log(unsigned displayLines, const std::string &fontName, int fontSize, const Color &color):
_maxMessages(displayLines),
_font(TTF_OpenFont(fontName.c_str(), fontSize)),
_color(color),
_compilationColor(color),
_valid(true){
    if (!_font){
        std::string err = TTF_GetError();
        _valid = false;
    }
}

Log::~Log(){
    while (!_messages.empty()){
        SDL_DestroyTexture(_messages.front());
        _messages.pop_front();
    }
    if (_font)
        TTF_CloseFont(_font);
}

void Log::operator()(const std::string &message, const Color *color){
    SDL_Surface *msgSurface = TTF_RenderText_Solid(_font, message.c_str(),
                                                   color ? *color : _color);
    if (!msgSurface)
        return;
    SDL_Renderer *renderer = Client::isClient ? Client::screen : Server::screen;
    SDL_Texture *msgTexture = SDL_CreateTextureFromSurface(renderer, msgSurface);
    SDL_FreeSurface(msgSurface);
    if (!msgTexture)
        return;

    _messages.push_back(msgTexture);
    if (_messages.size() > _maxMessages){
        SDL_DestroyTexture(_messages.front());
        _messages.pop_front();
    }
}

void Log::draw(SDL_Renderer *renderer, int x, int y) const{
    if (!_valid)
        return;
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    
    for (std::list<SDL_Texture *>::const_iterator it = _messages.begin(); it != _messages.end(); ++it) {
        SDL_Texture *msgSurface = *it;
        SDL_QueryTexture(msgSurface, 0, 0, &rect.w, &rect.h);
        SDL_RenderCopy(renderer, msgSurface, 0, &rect);
        rect.y += rect.h;
    }

}
