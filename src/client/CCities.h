#pragma once

// Stores a client's information about all cities in the game, so that it can
// inform the user about them.
class CCities {
 public:
  using Container = std::map<std::string, MapPoint>;

  void add(const std::string& cityName, const MapPoint& location) {
    _container[cityName] = location;
  }
  void remove(const std::string& cityName) { _container.erase(cityName); }
  size_t count() const { return _container.size(); }

  Container::const_iterator begin() const { return _container.begin(); }
  Container::const_iterator end() const { return _container.end(); }

 private:
  Container _container;
};
