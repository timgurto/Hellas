#include "Args.h"

Args::Args(int argc, char* argv[]){
    for (int i = 1; i != argc; ++i){
        if (argv[i][0] != '-')
            // Ill-formed argument; skip
            continue;
        std::string key = argv[i]+1;

        // Look ahead to see if next arg is a value rather than a key
        if (argc > i+1 && argv[i+1][0] != '-') {
            ++i;
            std::string value = argv[i];
            _args[key] = value;
        }else
            _args[key] = "";
    }
}

bool Args::contains(const std::string &key) const{
    return _args.find(key) != _args.end();
}

std::string Args::getString(const std::string &key) const{
    if (contains(key))
        return _args.find(key)->second;
    else
        return "";
}

int Args::getInt(const std::string &key) const{
    if (contains(key))
        return atoi(_args.find(key)->second.c_str());
    else
        return 0;
}
