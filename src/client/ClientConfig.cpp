#include "ClientConfig.h"

#include "../XmlReader.h"

void ClientConfig::loadFromFile(const std::string &filename) {
  auto xr = XmlReader::FromFile("client-config.xml");

  auto elem = xr.findChild("chatLog");
  xr.findAttr(elem, "width", chatW);
  xr.findAttr(elem, "height", chatH);

  elem = xr.findChild("gameFont");
  xr.findAttr(elem, "filename", fontFile);
  xr.findAttr(elem, "size", fontSize);
  xr.findAttr(elem, "offset", fontOffset);
  xr.findAttr(elem, "height", textHeight);

  elem = xr.findChild("castBar");
  xr.findAttr(elem, "y", castBarY);
  xr.findAttr(elem, "w", castBarW);
  xr.findAttr(elem, "h", castBarH);

  elem = xr.findChild("loginScreen");
  xr.findAttr(elem, "frontX", loginFrontOffset.x);
  xr.findAttr(elem, "frontY", loginFrontOffset.y);

  elem = xr.findChild("server");
  xr.findAttr(elem, "hostDirectory", serverHostDirectory);
}
