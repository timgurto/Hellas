#pragma once

#include <map>

class Object;
class User;

struct Action {
    using Function = void(*)(const Object &obj, User &performer);
    using FunctionMap = std::map<std::string, Function>;

    static FunctionMap functionMap;

    Function function = nullptr;
};
