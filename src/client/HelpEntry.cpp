#include "HelpEntry.h"

void HelpEntry::addParagraph(const std::string &heading, const std::string &text) {
    Paragraph p;
    p.heading = heading;
    p.text = text;
    _paragraphs.push_back(p);
}

