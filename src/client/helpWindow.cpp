#include "Client.h"
#include "ui/Line.h"
#include "../XmlReader.h"

extern Renderer renderer;

void loadHelpEntries(HelpEntries &entries);

void Client::initializeHelpWindow() {

    loadHelpEntries(_helpEntries);

    const px_t
        WIN_WIDTH = 350,
        WIN_HEIGHT = 250,
        WIN_X = (640 - WIN_WIDTH) / 2,
        WIN_Y = (360 - WIN_HEIGHT) / 2;

    _helpWindow = Window::WithRectAndTitle(
        Rect(WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT), "Help");

    // Topic list
    const px_t
        TOPIC_W = 60,
        TOPIC_BORDER = 2,
        TOPIC_GAP = 2;
    auto *topicList = new List({ TOPIC_BORDER, TOPIC_BORDER,
            TOPIC_W - TOPIC_BORDER * 2, WIN_HEIGHT - TOPIC_BORDER * 2},
            Element::TEXT_HEIGHT + TOPIC_GAP);
    for (auto &entry : _helpEntries) {
        topicList->addChild(new Label({}, entry.name()));
    }
    _helpWindow->addChild(topicList);

    // Divider
    const px_t
        LINE_X = TOPIC_W + TOPIC_BORDER * 2;
    _helpWindow->addChild(new Line(LINE_X, 0, WIN_HEIGHT, Element::VERTICAL));

    // Help text
    const px_t
        TEXT_GAP = 2,
        TEXT_X = LINE_X + 2 + TEXT_GAP,
        TEXT_Y = TEXT_GAP,
        TEXT_W = WIN_WIDTH - TEXT_X - TEXT_GAP,
        TEXT_H = WIN_HEIGHT - TEXT_GAP;
    auto *helpText = new List({ TEXT_X, TEXT_Y, TEXT_W, TEXT_H });
    _helpWindow->addChild(helpText);

    _helpEntries.draw("Cities", helpText);
}

void loadHelpEntries(HelpEntries &entries) {
    entries.clear();
    
    auto xr = XmlReader("Data/help.xml");
    if (!xr)
        return;
    for (auto elem : xr.getChildren("entry")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
            continue;
        auto newEntry = HelpEntry{ id, name };

        for (auto paragraph : xr.getChildren("paragraph", elem)) {
            auto order = size_t{};
            if (!xr.findAttr(paragraph, "order", order))
                continue;
            auto text = std::string{};
            if (!xr.findAttr(paragraph, "text", text))
                continue;
            auto heading = std::string{ "" };
            xr.findAttr(paragraph, "heading", heading);
            newEntry.addParagraph(order, heading, text);
        }
        entries.add(newEntry);
    }
}
