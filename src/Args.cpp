#include <cstdlib>

#include "Args.h"

void Args::init(int argc, char* argv[]){
    for (int i = 1; i != argc; ++i){
        if (argv[i][0] != '-')
            // Ill-formed argument; skip
            continue;
        const std::string key = argv[i]+1;

        // Look ahead to see if next arg is a value rather than a key
        if (argc > i+1 && argv[i+1][0] != '-') {
            ++i;
            std::string value = argv[i];
            _args[key] = value;
        }else
            _args[key] = "";
    }
}

void Args::add(const std::string &key, const std::string &value){
    _args[key] = value;
}

void Args::remove(const std::string &key){
    auto it = _args.find(key);
    if (it != _args.end())
        _args.erase(it);
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

std::ostream &operator<<(std::ostream &lhs, const Args &rhs){
    for (std::map<std::string, std::string>::const_iterator it = rhs._args.begin();
        it != rhs._args.end();){
        lhs << it->first;
        if (it->second != "")
            lhs << '=' << it->second;
        if (++it != rhs._args.end())
            lhs << ' ';
    }
    return lhs;
}
