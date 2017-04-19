#include <sstream>

#include "LogConsole.h"
#include "Server.h"

LogConsole::LogConsole(const std::string &logFileName):
Log(logFileName),
_quiet(false)
{}

static const std::string &colorCode(const Color &color = Color::NO_KEY){
    static std::map<Color, std::string> colorCodes;
    auto ret = colorCodes.insert(std::make_pair(color, std::string()));
    if (ret.second) { // Insert new color
        if (color == Color::NO_KEY)
            ret.first->second = "\x1B[0m"; // Return to default color
        else {
            std::ostringstream oss;
            oss << "\x1B[38;2;"
                << static_cast<int>(color.r()) << ';'
                << static_cast<int>(color.g()) << ';'
                << static_cast<int>(color.b()) << 'm';
            ret.first->second = oss.str();
        }
    }
    return ret.first->second;
}

void LogConsole::operator()(const std::string &message, const Color &color){
    writeLineToFile(message);
    if (_quiet)
        return;
    std::cout << colorCode(color) << message << colorCode() << std::endl;
}

LogConsole &LogConsole::operator<<(const std::string &val) {
    writeToFile(val);
    if (_quiet)
        return *this;
    std::cout << val << std::flush;

    return *this;
}

LogConsole &LogConsole::operator<<(const Color &c) {
    if (_quiet)
        return *this;
    std::cout << colorCode(c);

    return *this;
}

LogConsole &LogConsole::operator<<(const LogSpecial &val) {
    if (val == endl)
        writeLineToFile("");

    if (_quiet)
        return *this;

    switch (val){
    case endl:
        std::cout << colorCode() << std::endl;
        break;

    case uncolor:
        std::cout << colorCode() << std::endl;
        break;
    }
    return *this;
}
    
