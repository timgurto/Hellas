#pragma once

#include <set>
#include <string>
#include <vector>

class List;

struct Paragraph {
  std::string heading;
  std::string text;
  size_t order;

  bool operator<(const Paragraph &rhs) const { return order < rhs.order; }
};

class HelpEntry {
 public:
  HelpEntry(size_t order, const std::string &name)
      : _order(order), _name(name) {}

  const std::string &name() const { return _name; }

  bool operator<(const HelpEntry &rhs) const { return _order < rhs._order; }

  void addParagraph(size_t order, const std::string &heading,
                    const std::string &text);
  void draw(List *page) const;

 private:
  size_t _order;
  std::string _name;

  std::set<Paragraph> _paragraphs;
};

class HelpEntries {
 public:
  using Container = std::set<HelpEntry>;
  void add(const HelpEntry &newEntry) { _container.insert(newEntry); }
  void clear() { _container.clear(); }

  Container::const_iterator begin() const { return _container.begin(); }
  Container::const_iterator end() const { return _container.end(); }

  void draw(const std::string &entryName, List *page) const;

 private:
  Container _container;
};
