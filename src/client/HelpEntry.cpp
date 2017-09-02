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
class Words {
public:
    void addWordsFromString(const std::string &paragraph, Color color = Color::FONT) {
        auto iss = std::istringstream(paragraph);
        while (iss) {
            auto word = std::string{};
            iss >> word;
            auto *label = new Label{ {0, 0, 0, Element::TEXT_HEIGHT}, word };
            label->matchW();
            if (color != Color::FONT)
                label->setColor(color);
            label->refresh();
            _container.push_back(label);
        }
    }
    void addNewLine(){ _container.push_back(nullptr); }
    void addBlankLine() { addNewLine(); addNewLine(); }

    void setIn(List *page) const {
        const px_t
            SPACE = 3;

        auto x = px_t{ 0 };
        auto *line = new Element();
        for (auto *word : _container) {
            // New line, instead of a word
            if (word == nullptr) {
                page->addChild(line);
                line = new Element();
                x = 0;
                continue;
            }

            auto lineWidthIncludingThisWord = x + word->width() + SPACE;
            if (lineWidthIncludingThisWord > page->width() - List::ARROW_W) {
                page->addChild(line);
                line = new Element();
                x = 0;
            }
            if (x > 0)
                x += SPACE;
            word->setPosition(x, 0);
            line->addChild(word);
            x += word->width();
        }
        page->addChild(line);
    }

private:
    std::vector<Label*> _container; // nullptr = new line
};

void HelpEntry::draw(List * page) const {
    // Compile words list
    Words words;
    bool pageIsEmpty = true;
    for (const auto &paragraph : _paragraphs) {
        if (!paragraph.heading.empty()) {
            if (!pageIsEmpty)
                words.addBlankLine();
            words.addWordsFromString(paragraph.heading, Color::HELP_TEXT_HEADING);
            words.addNewLine();
        }

        words.addWordsFromString(paragraph.text);
        pageIsEmpty = false;
    }

    page->clearChildren();
    auto *title = new Label({}, "-- " + _name + " --",  Element::CENTER_JUSTIFIED);
    title->setColor(Color::HELP_TEXT_HEADING);
    page->addChild(title);
    page->addChild(new Element);
    words.setIn(page);
}

void HelpEntries::draw(const std::string & entryName, List * page) const {
    auto it = _container.find({ "", entryName });
    if (it == _container.end())
        return;
    it->draw(page);
}
