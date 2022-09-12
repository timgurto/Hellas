#include "Options.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "Client.h"
#include "ui/CheckBox.h"
#include "ui/List.h"

void Options::save() {
  auto xw = XmlWriter{m_filePath};

  auto *elem = xw.addChild("video");
  xw.setAttr(elem, "fullScreen", video.fullScreen);

  elem = xw.addChild("audio");
  xw.setAttr(elem, "enableSFX", audio.enableSFX);

  elem = xw.addChild("parental");
  xw.setAttr(elem, "showCustomNames", parental.showCustomNames);

  xw.publish();
}

void Options::load() {
  auto xr = XmlReader::FromFile(m_filePath);

  auto *elem = xr.findChild("video");
  xr.findAttr(elem, "fullScreen", video.fullScreen);

  elem = xr.findChild("audio");
  xr.findAttr(elem, "enableSFX", audio.enableSFX);

  elem = xr.findChild("parental");
  xr.findAttr(elem, "showCustomNames", parental.showCustomNames);
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

  addSection("Video");
  addBoolOption("Full screen (requires restart)", options.video.fullScreen);
  addGap();

  addSection("Audio");
  addBoolOption("Enable sound effects", options.audio.enableSFX);
  addGap();

  addSection("Parental Controls");
  addBoolOption("Show others' custom object names",
                options.parental.showCustomNames);
}
