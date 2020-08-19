#pragma once

#include "../types.h"

class User;
class Entity;

/*
 * Describes who will get credit for killing an entity.  Includes information
 * used for external logging of the kill.
 */
class Tagger {
 public:
  Tagger& operator=(User& user);
  bool operator==(const User& rhs) const;
  bool operator==(const Entity& rhs) const;
  void clear();
  operator bool() const;
  User* asUser() const;
  std::string username() const;

  void onDisconnect();

 private:
  void findUser() const;
  bool isTagged() const;
  mutable User* _user{nullptr};

  // Saved in case user disconnects
  std::string _username;
};
