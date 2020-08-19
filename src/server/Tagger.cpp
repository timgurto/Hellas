#include "Tagger.h"

#include "Server.h"
#include "User.h"

Tagger& Tagger::operator=(User& user) {
  _user = &user;
  _username = user.name();
  return *this;
}

bool Tagger::operator==(const User& rhs) const {
  if (asUser() == &rhs) return true;
  return _username == rhs.name();
}

bool Tagger::operator==(const Entity& rhs) const {
  if (asUser() == &rhs) return true;
  auto* asUser = dynamic_cast<const User*>(&rhs);
  if (!asUser) return false;
  return _username == asUser->name();
}

void Tagger::clear() {
  _username = {};
  _user = nullptr;
}

Tagger::operator bool() const { return isTagged(); }

User* Tagger::asUser() const {
  if (!_user) findUser();
  return _user;
}

std::string Tagger::username() const { return _username; }

void Tagger::onDisconnect() { _user = nullptr; }

void Tagger::findUser() const {
  if (!isTagged()) return;
  _user = Server::instance().getUserByName(_username);
}

bool Tagger::isTagged() const { return !_username.empty(); }
