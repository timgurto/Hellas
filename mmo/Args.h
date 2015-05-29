#ifndef ARGS_H
#define ARGS_H

#include <map>
#include <string>

// A class to simplify the handling of command-line arguments
class Args{
public:
    Args(int argc, char* argv[]);
    bool contains(const std::string &key) const;

    // These functions will return "" or 0 if the key is not found, so use contains() first for accuracy.
    std::string getString(const std::string &key) const;
    int getInt(const std::string &key) const;

private:
    /*
    Example:
    Original arguments: "-key value -keyOnly"
    _args["key"] == "value"
    _args["keyOnly"] == ""
    */
    std::map<std::string, std::string> _args;
};

#endif
