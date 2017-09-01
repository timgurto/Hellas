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
    auto *title = new Label({}, "-- " + _name + " --",  Element::CENTER_JUSTIFIED);
    title->setColor(Color::HELP_TEXT_HEADING);
    page->addChild(title);
    page->addChild(new Element);

    bool pageIsEmpty = true;

    for (const auto &paragraph : _paragraphs) {
        // Draw heading
        if (!paragraph.heading.empty()) {
            if (!pageIsEmpty)
                page->addChild(new Element);
            auto *heading = new Label({}, paragraph.heading);
            heading->setColor(Color::HELP_TEXT_HEADING);
            page->addChild(heading);
        }

        // Draw paragraph
        page->addChild(new Label({}, paragraph.text));
        pageIsEmpty = false;
    }
}

void HelpEntries::draw(const std::string & entryName, List * page) const {
    auto it = _container.find({ "", entryName });
    if (it == _container.end())
        return;
    it->draw(page);
}
