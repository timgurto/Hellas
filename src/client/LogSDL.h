#ifndef LOG_SDL_H
#define LOG_SDL_H

#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

#include "../Log.h"
#include "Texture.h"

// A message log that uses SDL to display colored text on the screen.
class LogSDL : public Log{
public:
    LogSDL(const std::string &logFileName = "");
    ~LogSDL() override;
    void operator()(const std::string &message, const Color &color = Color::FONT) override;

    template<typename T>
    LogSDL &operator<<(const T &val) {
        _oss << val;
        return *this;
    }
    LogSDL &operator<<(const std::string &val) override;
    LogSDL &operator<<(const LogSpecial &val) override;
    LogSDL &operator<<(const Color &c) override;

    void draw(px_t x = 0, px_t y = 0) const;

private:
    std::ostringstream _oss;
    Color _compilationColor; // for use while compiling a message
    std::ofstream _logFile;
};

#endif
