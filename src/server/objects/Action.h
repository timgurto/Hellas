#pragma once

#include <map>
#include <string>

class Object;
class ServerItem;
class User;

struct Action {
  struct Args {
    std::string textFromUser;
    double d1 = 0;
    double d2 = 0;
    double d3 = 0;
  };
  using Function = bool (*)(const Object &obj, User &performer,
                            const Args &args);
  using FunctionMap = std::map<std::string, Function>;

  static FunctionMap functionMap;

  Function function{nullptr};
  Args args;
  const ServerItem *cost{nullptr};
};

// More specific functions for certain stimuli.
struct CallbackAction {
  using Function = void (*)(const Object &obj);
  using FunctionMap = std::map<std::string, Function>;

  static FunctionMap functionMap;

  Function function = nullptr;
};
