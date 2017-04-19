#ifndef ARGS_H
#define ARGS_H

#include <map>
#include <iostream>
#include <string>

// A class to simplify the handling of command-line arguments
class Args{
public:
    void init(int argc, char* argv[]);
    bool contains(const std::string &key) const;
    
    void add(const std::string &key, const std::string &value = "");
    void remove(const std::string &key);

    // These functions will return "" or 0 if the key is not found, so use contains() first for accuracy.
    std::string getString(const std::string &key) const;
    int getInt(const std::string &key) const;

    friend std::ostream &operator<<(std::ostream &lhs, const Args &rhs);

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
