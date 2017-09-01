#include "HelpEntry.h"
#include "ui/Label.h"
#include "ui/List.h"

void HelpEntry::addParagraph(const std::string &heading, const std::string &text) {
    Paragraph p;
    p.heading = heading;
    p.text = text;
    _paragraphs.push_back(p);
}

void HelpEntry::draw(List * page) const {
    page->clearChildren();
    for (const auto &paragraph : _paragraphs) {
        if (!paragraph.heading.empty()) {
            page->addChild(new Element);
            page->addChild(new Label({}, paragraph.heading));
        }
        page->addChild(new Label({}, paragraph.text));
    }
}

void HelpEntries::draw(const std::string & entryName, List * page) const {
    auto it = _container.find({ "", entryName });
    if (it == _container.end())
        return;
    it->draw(page);
}
