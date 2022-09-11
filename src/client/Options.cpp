#include "Options.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "Client.h"
#include "ui/CheckBox.h"
#include "ui/List.h"

void Options::save() {
  auto xw = XmlWriter{m_filePath};

  auto *elemGraphics = xw.addChild("graphics");
  xw.setAttr(elemGraphics, "fullScreen", graphics.fullScreen);

  xw.publish();
}

void Options::load() {
  auto xr = XmlReader::FromFile(m_filePath);

  auto *elemGraphics = xr.findChild("graphics");
  xr.findAttr(elemGraphics, "fullScreen", graphics.fullScreen);
}

void Options::getFilePath() {
  char *appDataPath = nullptr;
  _dupenv_s(&appDataPath, nullptr, "LOCALAPPDATA");
  if (!appDataPath) return;
  m_filePath = std::string{appDataPath} + "\\Hellas\\options.xml"s;
  free(appDataPath);
}

extern Options options;

void Client::initialiseOptionsWindow() {
  const px_t WIN_WIDTH = 200, WIN_HEIGHT = 250, ITEM_HEIGHT = 12;
  _optionsWindow = Window::WithRectAndTitle({0, 0, WIN_WIDTH, WIN_HEIGHT},
                                            "Options"s, mouse());
  _optionsWindow->center();

  auto *contents = new List({1, 0, _optionsWindow->contentWidth() - 1,
                             _optionsWindow->contentHeight()},
                            ITEM_HEIGHT);
  _optionsWindow->addChild(contents);

  auto addSection = [contents](std ::string name) {
    auto *label = new Label(
        {0, 0, contents->contentWidth(), contents->childHeight()}, name);
    label->setColor(Color::WINDOW_HEADING);
    contents->addChild(label);
  };

  auto addGap = [contents]() { contents->addChild(new Element); };

  auto addBoolOption = [this, contents](std::string description,
                                        bool &linkedValue) {
    auto *checkbox = new CheckBox(
        *this, {0, 0, contents->contentWidth(), contents->childHeight()},
        linkedValue, description);
    checkbox->onChange([](Client &) { options.save(); });
    contents->addChild(checkbox);
  };

  addSection("Graphics");
  addBoolOption("Full screen (requires restart)", options.graphics.fullScreen);
}
