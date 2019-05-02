#include "Sprite.h"

#include <cassert>

#include "../util.h"
#include "Client.h"
#include "Renderer.h"
#include "Tooltip.h"

extern Renderer renderer;

const std::string Sprite::EMPTY_NAME = "";

Sprite::Sprite(const SpriteType *type, const MapPoint &location)
    : _yChanged(false),
      _type(type),
      _location(location),
      _locationOnServer(location),
      _toRemove(false) {}

ScreenRect Sprite::drawRect() const {
  assert(_type != nullptr);
  auto typeDrawRect = _type->drawRect();
  auto drawRect = typeDrawRect + toScreenPoint(_location);
  return drawRect;
}

double Sprite::speed() const {
  const auto &client = Client::instance();
  if (this == &client.character()) return client._stats.speed;
  return Client::MOVEMENT_SPEED;
}

void Sprite::draw(const Client &client) const {
  if (shouldDrawShadow()) {
    const auto &shadow = type()->shadow();
    auto shadowX = type()->hasCustomShadowWidth()
                       ? -type()->customShadowWidth() / 2
                       : toInt(type()->drawRect().x * SpriteType::SHADOW_RATIO);
    auto shadowY = -toInt(shadow.height() / 2.0);
    auto shadowPosition = toScreenPoint(_location) +
                          ScreenPoint{shadowX, shadowY} + client.offset();
    shadow.draw(shadowPosition);
  }

  const Texture &imageToDraw =
      client.currentMouseOverEntity() == this ? highlightImage() : image();
  if (imageToDraw)
    imageToDraw.draw(drawRect() + client.offset());
  else {
    renderer.setDrawColor(Color::MISSING_IMAGE);
    auto drawRect =
        toScreenRect(MapRect{_location.x - 5, _location.y - 5, 10, 10});
    renderer.fillRect(drawRect + client.offset());
  }
}

void Sprite::drawName() const {
  const auto &client = Client::instance();

  auto text = name();
  if (!additionalTextInName().empty()) text += " "s + additionalTextInName();

  const auto nameLabel = Texture{client.defaultFont(), text, nameColor()};
  const auto nameOutline =
      Texture{client.defaultFont(), text, Color::UI_OUTLINE};
  auto namePosition = toScreenPoint(location()) + client.offset();
  namePosition.y -= height();
  namePosition.y -= 16;
  namePosition.x -= nameLabel.width() / 2;
  for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
      nameOutline.draw(namePosition + ScreenPoint(x, y));
  nameLabel.draw(namePosition);
}

void Sprite::update(double delta) {
  auto &client = Client::instance();

  driftTowardsServerLocation(delta);

  if (!shouldAddParticles()) return;

  for (auto &p : _type->particles()) {
    auto particleX = _location.x + p.offset.x;
    auto particleY = bottomEdge();
    auto altitude = -p.offset.y + (bottomEdge() - _location.y);
    client.addParticlesWithCustomAltitude(altitude, p.profile,
                                          {particleX, particleY}, delta);
  }
}

double Sprite::bottomEdge() const {
  if (_type != nullptr)
    return _location.y + _type->drawRect().y + _type->height();
  else
    return _location.y;
}

void Sprite::location(const MapPoint &loc) {
  const double oldY = _location.y;
  _location = loc;
  if (_location.y != oldY) _yChanged = true;
}

bool Sprite::collision(const MapPoint &p) const {
  return ::collision(toScreenPoint(p), drawRect());
}

bool Sprite::mouseIsOverRealPixel(const MapPoint &p) const {
  auto pos = toScreenPoint(p);
  const auto &dr = drawRect();
  pos.x -= dr.x;
  pos.y -= dr.y;

  auto pixel = image().getPixel(pos.x, pos.y);
  if (pixel == Color::MAGENTA) return false;

  return true;
}

const Texture &Sprite::cursor(const Client &client) const {
  return client.cursorNormal();
}

const Tooltip &Sprite::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();
  return Tooltip::noTooltip();
}

void Sprite::driftTowardsServerLocation(double delta) {
  if (!_serverHasOrderedACorrection) return;

  const double maxLegalDistance = delta * speed();
  auto newLocation =
      interpolate(location(), _locationOnServer, maxLegalDistance);
  location(newLocation);

  if (newLocation == _locationOnServer) _serverHasOrderedACorrection = false;
}
