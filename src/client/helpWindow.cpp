#include "Client.h"
#include "../XmlReader.h"

extern Renderer renderer;

void loadHelpEntries(HelpEntries &entries);

void Client::initializeHelpWindow() {

    loadHelpEntries(_helpEntries);

    const px_t
        WIN_WIDTH = 400,
        WIN_HEIGHT = 250,
        WIN_X = (640 - WIN_WIDTH) / 2,
        WIN_Y = (360 - WIN_HEIGHT) / 2;

    _helpWindow = Window::WithRectAndTitle(
        Rect(WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT), "Help");

    // Topic list
    const px_t
        TOPIC_W = 100,
        TOPIC_BORDER = 2,
        TOPIC_GAP = 2;
    auto *topicList = new List({ TOPIC_BORDER, TOPIC_BORDER,
            TOPIC_W - TOPIC_BORDER * 2, WIN_HEIGHT - TOPIC_BORDER * 2},
            Element::TEXT_HEIGHT + TOPIC_GAP);
    for (auto &entry : _helpEntries) {
        topicList->addChild(new Label({}, entry.name()));
    }

    _helpWindow->addChild(topicList);
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

        for (auto elem : xr.getChildren("paragraph")) {
            std::string text, heading;
            if (!xr.findAttr(elem, "text", text))
                continue;
            xr.findAttr(elem, "heading", heading);
            newEntry.addParagraph(heading, text);
        }
        entries.push_back(newEntry);
    }
}
