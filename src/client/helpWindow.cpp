#include <cassert>

#include "../XmlReader.h"
#include "Client.h"
#include "ui/Line.h"
#include "ui/Window.h"

extern Renderer renderer;

void loadHelpEntries(HelpEntries &entries);

void showTopic();

void Client::initializeHelpWindow() {
  loadHelpEntries(_helpEntries);

  const px_t WIN_WIDTH = 350, WIN_HEIGHT = 250, WIN_X = (640 - WIN_WIDTH) / 2,
             WIN_Y = (360 - WIN_HEIGHT) / 2;

  _helpWindow =
      Window::WithRectAndTitle({WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT}, "Help");

  // Topic list
  const px_t TOPIC_W = 85, TOPIC_BORDER = 2, TOPIC_GAP = 2;
  auto *topicList =
      new ChoiceList({TOPIC_BORDER, TOPIC_BORDER, TOPIC_W - TOPIC_BORDER * 2,
                      WIN_HEIGHT - TOPIC_BORDER * 2},
                     Element::TEXT_HEIGHT + TOPIC_GAP);
  for (auto &entry : _helpEntries) {
    auto topic = new Element;
    topic->id(entry.name());
    topicList->addChild(topic);
    auto *topicLabel = new Label({1, 1, 0, Element::TEXT_HEIGHT}, entry.name());
    topicLabel->matchW();
    topicLabel->refresh();
    topic->addChild(topicLabel);
  }
  topicList->verifyBoxes();
  topicList->id("topicList");
  topicList->onSelect = showTopic;
  // topicList->setLeftMouseDownFunction(showTopic, _helpWindow);
  _helpWindow->addChild(topicList);

  // Divider
  const px_t LINE_X = TOPIC_W + TOPIC_BORDER * 2;
  _helpWindow->addChild(new Line(LINE_X, 0, WIN_HEIGHT, Element::VERTICAL));

  // Help text
  const px_t TEXT_GAP = 1, TEXT_X = LINE_X + 2 + TEXT_GAP, TEXT_Y = TEXT_GAP,
             TEXT_W = WIN_WIDTH - TEXT_X - TEXT_GAP,
             TEXT_H = WIN_HEIGHT - TEXT_GAP * 2;
  auto *helpText = new List({TEXT_X, TEXT_Y, TEXT_W, TEXT_H});
  helpText->id("helpText");
  _helpWindow->addChild(helpText);
}

void Client::showHelpTopic(const std::string &topic) {
  auto topicList =
      dynamic_cast<ChoiceList *>(_helpWindow->findChild("topicList"));
  topicList->manuallySelect(topic);
  _helpWindow->show();
}

void showTopic() {
  auto &helpWindow = Client::instance().helpWindow();
  auto *helpText = dynamic_cast<List *>(helpWindow.findChild("helpText"));
  auto *topicList =
      dynamic_cast<ChoiceList *>(helpWindow.findChild("topicList"));
  if (helpText == nullptr || topicList == nullptr) assert(false);
  const auto &selectedTopic = topicList->getSelected();
  const auto &helpEntries = Client::instance().helpEntries();
  if (selectedTopic.empty())
    helpText->clearChildren();
  else
    helpEntries.draw(selectedTopic, helpText);
}

void loadHelpEntries(HelpEntries &entries) {
  entries.clear();

  auto xr = XmlReader::FromFile("Data/help.xml");
  if (!xr) return;
  for (auto elem : xr.getChildren("entry")) {
    std::string name;
    if (!xr.findAttr(elem, "name", name)) continue;
    auto newEntry = HelpEntry{name};

    for (auto paragraph : xr.getChildren("paragraph", elem)) {
      auto text = ""s;
      if (!xr.findAttr(paragraph, "text", text)) continue;
      auto heading = ""s;
      xr.findAttr(paragraph, "heading", heading);
      newEntry.addParagraph(heading, text);
    }
    entries.add(newEntry);
  }
}
