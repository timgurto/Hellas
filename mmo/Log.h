#ifndef LOG_H
#define LOG_H

#include <list>
#include <sstream>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

// A message log which accepts, queues and displays messages to the screen
class Log{
public:
    Log();
    Log(unsigned displayLines, const std::string &fontName = "trebuc.ttf", int fontSize = 16);
    ~Log();
    void operator()(const std::string &message);
    void draw(SDL_Surface *targetSurface, int x = 0, int y = 0);

    enum LogEndType{endl};

    // Usage: log << "1" << 2 << endl;
    // Does not add the completed message to the log until it receives endl.
    template<typename T>
    Log &operator<<(const T &val) {
        _oss << val;
        return *this;
    }
    template<>
    Log &operator<<(const LogEndType &val) {
        operator()(_oss.str());
        _oss.str("");
        return *this;
    }

private:
    std::list<SDL_Surface *> _messages;
    unsigned _maxMessages;
    TTF_Font *_font;
    bool _valid; // false if an error has occurred
    std::ostringstream _oss;
};

#endif
