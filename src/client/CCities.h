#pragma once

// Stores a client's information about all cities in the game, so that it can
// inform the user about them.
class CCities {
 public:
  struct City {
    std::string name;
  };

  using Container = std::vector<City>;

  void add() { _container.push_back({}); }
  size_t count() const { return _container.size(); }

  Container::const_iterator begin() const { return _container.begin(); }
  Container::const_iterator end() const { return _container.end(); }

 private:
  Container _container;
};
