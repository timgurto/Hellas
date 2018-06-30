#ifndef ARGS_H
#define ARGS_H

#include <iostream>
#include <map>
#include <string>

// A class to simplify the handling of command-line arguments
class Args {
 public:
  typedef std::string Key, Value;

  void init(int argc, char *argv[]);
  bool contains(const Key &key) const;

  void add(const Key &key, const Value &value = "");
  void remove(const Key &key);

  // These functions will return "" or 0 if the key is not found, so use
  // contains() first for accuracy.
  std::string getString(const Key &key) const;
  int getInt(const Key &key) const;

  friend std::ostream &operator<<(std::ostream &lhs, const Args &rhs);

 private:
  /*
  Example:
  Original arguments: "-key value -keyOnly"
  _args["key"] == "value"
  _args["keyOnly"] == ""
  */
  std::map<Key, Value> _args;
};

#endif
