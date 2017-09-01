#include "HelpEntry.h"
#include "ui/Label.h"
#include "ui/List.h"

void HelpEntry::addParagraph(size_t order, const std::string &heading, const std::string &text) {
    Paragraph p;
    p.order = order;
    p.heading = heading;
    p.text = text;
    _paragraphs.insert(p);
}

void HelpEntry::draw(List * page) const {
    page->clearChildren();
    for (const auto &paragraph : _paragraphs) {
        if (!paragraph.heading.empty()) {
            if (_paragraphs.size() > 1)
                page->addChild(new Element);
            auto *heading = new Label({}, paragraph.heading);
            heading->setColor(Color::HELP_TEXT_HEADING);
            page->addChild(heading);
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
