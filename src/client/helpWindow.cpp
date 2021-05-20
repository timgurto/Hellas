#include <cassert>

#include "../XmlReader.h"
#include "Client.h"
#include "ui/Line.h"
#include "ui/Window.h"

extern Renderer renderer;

void showTopic(Client &client);

void Client::HelpWindow::initialise(Client &client) {
  loadEntries();

  const px_t WIN_WIDTH = 400, WIN_HEIGHT = 250, WIN_X = (640 - WIN_WIDTH) / 2,
             WIN_Y = (360 - WIN_HEIGHT) / 2;

  window = Window::WithRectAndTitle({WIN_X, WIN_Y, WIN_WIDTH, WIN_HEIGHT},
                                    "Help", client.mouse());

  // Topic list
  const px_t TOPIC_W = 100, TOPIC_BORDER = 2, TOPIC_GAP = 2;
  auto *topicList =
      new ChoiceList({TOPIC_BORDER, TOPIC_BORDER, TOPIC_W - TOPIC_BORDER * 2,
                      WIN_HEIGHT - TOPIC_BORDER * 2},
                     Element::TEXT_HEIGHT + TOPIC_GAP, client);
  for (auto &entry : entries) {
    auto topic = new Element;
    topic->id(entry.name());
    topicList->addChild(topic);
    auto *topicLabel =
        new Label({1, 1, topic->width(), Element::TEXT_HEIGHT}, entry.name());
    topic->addChild(topicLabel);
  }
  topicList->verifyBoxes();
  topicList->id("topicList");
  topicList->onSelect = showTopic;
  // topicList->setLeftMouseDownFunction(showTopic, _helpWindow);
  window->addChild(topicList);

  // Divider
  const px_t LINE_X = TOPIC_W + TOPIC_BORDER * 2;
  window->addChild(new Line(LINE_X, 0, WIN_HEIGHT, Element::VERTICAL));

  // Help text
  const px_t TEXT_GAP = 1, TEXT_X = LINE_X + 2 + TEXT_GAP, TEXT_Y = TEXT_GAP,
             TEXT_W = WIN_WIDTH - TEXT_X - TEXT_GAP,
             TEXT_H = WIN_HEIGHT - TEXT_GAP * 2;
  auto *helpText = new List({TEXT_X, TEXT_Y, TEXT_W, TEXT_H});
  helpText->id("helpText");
  window->addChild(helpText);
}

void Client::HelpWindow::showHelpTopic(const std::string &topic) {
  auto topicList = dynamic_cast<ChoiceList *>(window->findChild("topicList"));
  topicList->manuallySelect(topic);
  window->show();
}

void showTopic(Client &client) {
  auto &helpWindow = client.helpWindow();
  auto *helpText = dynamic_cast<List *>(helpWindow.findChild("helpText"));
  auto *topicList =
      dynamic_cast<ChoiceList *>(helpWindow.findChild("topicList"));
  if (helpText == nullptr || topicList == nullptr) assert(false);
  const auto &selectedTopic = topicList->getSelected();
  const auto &helpEntries = client.helpEntries();
  if (selectedTopic.empty())
    helpText->clearChildren();
  else
    helpEntries.draw(selectedTopic, helpText);
}

void Client::HelpWindow::loadEntries() {
  entries.clear();

  auto xr = XmlReader::FromFile("Data/help.xml");
  if (!xr) return;
  for (auto elem : xr.getChildren("entry")) {
    std::string name;
    if (!xr.findAttr(elem, "name", name)) continue;
    auto newEntry = HelpEntry{name};

    for (auto paragraph : xr.getChildren("p", elem)) {
      auto text = ""s;
      if (!xr.findAttr(paragraph, "text", text)) continue;
      auto heading = ""s;
      xr.findAttr(paragraph, "heading", heading);
      newEntry.addParagraph(heading, text);
    }
    entries.add(newEntry);
  }
}
