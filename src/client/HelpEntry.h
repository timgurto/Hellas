#pragma once

#include <string>
#include <vector>

struct Paragraph {
    std::string heading;
    std::string text;
};

class HelpEntry {
public:
    HelpEntry(const std::string &id, const std::string &name) : _id(id), _name(name) {}
    void addParagraph(const std::string &heading, const std::string &text) {
        Paragraph p;
        p.heading = heading;
        p.text = text;
        _paragraphs.push_back(p);
    }
    const std::string &name() const { return _name; }

private:
    std::string _id;
    std::string _name;

    std::vector<Paragraph> _paragraphs;
};

using HelpEntries = std::vector<HelpEntry>;
