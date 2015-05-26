#ifndef LOG_H
#define LOG_H

#include <list>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

// A message log which accepts, queues and displays messages to the screen
class Log{
public:
    Log();
    Log(unsigned displayLines, const std::string &fontName = "consola.ttf", int fontSize = 18);
    ~Log();
    void operator()(const std::string &message);
    void draw(SDL_Surface *targetSurface, int x = 0, int y = 0);

private:
    std::list<std::string> _messages;
    unsigned _maxMessages;
    TTF_Font *_font;
    SDL_sem *_lock;
    bool _valid; // false if an error has occurred
};

#endif
