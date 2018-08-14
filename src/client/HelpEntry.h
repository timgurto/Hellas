#pragma once

#include <string>
#include <vector>

class List;

struct Paragraph {
  std::string heading;
  std::string text;
};

class HelpEntry {
 public:
  HelpEntry(const std::string &name) : _name(name) {}

  const std::string &name() const { return _name; }

  void addParagraph(const std::string &heading, const std::string &text);
  void draw(List *page) const;

 private:
  std::string _name;

  std::vector<Paragraph> _paragraphs;
};

class HelpEntries {
 public:
  using Container = std::vector<HelpEntry>;
  void add(const HelpEntry &newEntry) { _container.push_back(newEntry); }
  void clear() { _container.clear(); }

  Container::const_iterator begin() const { return _container.begin(); }
  Container::const_iterator end() const { return _container.end(); }

  void draw(const std::string &entryName, List *page) const;

 private:
  Container _container;
};
