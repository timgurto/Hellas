#pragma once

#include <map>
#include <string>

enum PeaceState {
  NO_PEACE_PROPOSED,
  PEACE_PROPOSED_BY_YOU,
  PEACE_PROPOSED_BY_HIM
};

class YourWars {
  using Container = std::map<std::string, PeaceState>;

 public:
  void add(const std::string &name);
  bool atWarWith(const std::string &name) const;
  void proposePeaceWith(const std::string &name);
  void peaceWasProposedBy(const std::string &name);
  void cancelPeaceOffer(const std::string &name);
  void remove(const std::string &name);

  Container::const_iterator begin() const { return _container.begin(); }
  Container::const_iterator end() const { return _container.end(); }

 private:
  Container _container;
};
