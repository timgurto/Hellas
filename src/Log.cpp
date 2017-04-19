#include "Log.h"

Log::Log(const std::string &logFileName):
_logFileName(logFileName){
    if (!logFileName.empty()) {
        std::ofstream of(logFileName);
        of.close();
    }
}
