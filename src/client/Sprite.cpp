#include "Sprite.h"

#include <cassert>

#include "../util.h"
#include "Client.h"
#include "Renderer.h"
#include "SpriteType.h"
#include "Tooltip.h"

extern Renderer renderer;

const std::string Sprite::EMPTY_NAME = "";
const ScreenPoint Sprite::HIGHLIGHT_OFFSET{-1, -1};
const double Sprite::ATTACK_ANIMATION_SPEED = 50.0;
const double Sprite::ATTACK_ANIMATION_DISTANCE = 10.0;

Sprite::Sprite(const SpriteType *type, const MapPoint &location, Client &client)
    : _client(client),
      _yChanged(false),
      _type(type),
      _location(location),
      _locationOnServer(location),
      _toRemove(false) {}

Sprite Sprite::YCoordOnly(double y) {
  return {nullptr, {0, y}, *(Client *)(nullptr)};
}

ScreenRect Sprite::drawRect() const {
  assert(_type != nullptr);
  auto typeDrawRect = _type->drawRect();
  auto drawRect = typeDrawRect + toScreenPoint(animationLocation());
  return drawRect;
}

bool Sprite::isCharacter() const { return this == &_client.character(); }

void Sprite::newLocationFromServer(const MapPoint &loc) {
  _locationOnServer = loc;
  _serverHasOrderedACorrection = true;

  onNewLocationFromServer();
}

double Sprite::speed() const {
  if (isCharacter()) return _client._stats.speed;
  return Client::MOVEMENT_SPEED;
}

void Sprite::draw() const {
  if (shouldDrawShadow()) drawShadow();

  auto shouldDrawHighlightInstead = _client.currentMouseOverEntity() == this;
  const Texture &imageToDraw =
      shouldDrawHighlightInstead ? getHighlightImage() : image();
  auto drawRect = this->drawRect() + _client.offset();
  if (shouldDrawHighlightInstead) drawRect += HIGHLIGHT_OFFSET;
  if (imageToDraw)
    imageToDraw.draw(drawRect.x, drawRect.y);
  else {
    renderer.setDrawColor(Color::MISSING_IMAGE);
    auto drawRect =
        toScreenRect(MapRect{_location.x - 5, _location.y - 5, 10, 10});
    renderer.fillRect(drawRect);
  }
}

void Sprite::drawShadow() const {
  const auto &shadow = type()->shadow();
  auto shadowX = type()->hasCustomShadowWidth()
                     ? -type()->customShadowWidth() / 2
                     : toInt(type()->drawRect().x * SpriteType::SHADOW_RATIO);
  auto shadowY = -toInt(shadow.height() / 2.0);
  auto shadowPosition = toScreenPoint(_location) +
                        ScreenPoint{shadowX, shadowY} + _client.offset();
  shadow.draw(shadowPosition);
}

void Sprite::drawName() const {
  auto text = name();
  if (!additionalTextInName().empty()) text += " "s + additionalTextInName();

  const auto nameLabel = Texture{_client.defaultFont(), text, nameColor()};
  const auto nameOutline =
      Texture{_client.defaultFont(), text, Color::UI_OUTLINE};
  auto namePosition = toScreenPoint(location()) + _client.offset();
  namePosition.y -= height();
  namePosition.y -= 16;
  namePosition.x -= nameLabel.width() / 2;
  for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
      nameOutline.draw(namePosition + ScreenPoint(x, y));
  nameLabel.draw(namePosition);
}

void Sprite::update(double delta) {
  driftTowardsServerLocation(delta);
  progressAttackAnimation(delta);

  if (!shouldAddParticles()) return;

  for (auto &p : _type->particles()) {
    auto particleX = _location.x + p.offset.x;
    auto particleY = bottomEdge();
    auto altitude = -p.offset.y + (bottomEdge() - _location.y);
    _client.addParticlesWithCustomAltitude(altitude, p.profile,
                                           {particleX, particleY}, delta);
  }
}

double Sprite::bottomEdge() const {
  if (!_type) return _location.y;

  auto height = _type->hasCustomDrawHeight() ? _type->customDrawHeight()
                                             : _type->height();
  return _location.y + _type->drawRect().y + height;
}

void Sprite::location(const MapPoint &loc) {
  const double oldY = _location.y;
  _location = loc;
  if (_location.y != oldY) _yChanged = true;

  onLocationChange();
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

void Sprite::animateAttackingTowards(const Sprite &target) {
  const auto towardsTarget = target.location() - location();
  const auto normalised = towardsTarget / towardsTarget.length();
  _offsetForAttackAnimation = normalised * ATTACK_ANIMATION_DISTANCE;
}

MapPoint Sprite::animationLocation() const {
  return _location + _offsetForAttackAnimation;
}

void Sprite::progressAttackAnimation(double delta) {
  if (_offsetForAttackAnimation == MapPoint{}) return;

  const auto distToMove = ATTACK_ANIMATION_SPEED * delta;
  _offsetForAttackAnimation =
      interpolate(_offsetForAttackAnimation, {}, distToMove);
}

const Texture &Sprite::cursor() const { return Client::images.cursorNormal; }

const Tooltip &Sprite::tooltip() const {
  if (_tooltip.hasValue()) return _tooltip.value();
  return Tooltip::noTooltip();
}

void Sprite::driftTowardsServerLocation(double delta) {
  if (!_serverHasOrderedACorrection) return;

  auto speedMultiplier = isCharacter() ? 1.5 : 1.1;
  const double correctionAmount = delta * speed() * speedMultiplier;
  auto newLocation =
      interpolate(location(), _locationOnServer, correctionAmount);
  location(newLocation);

  if (newLocation == _locationOnServer) _serverHasOrderedACorrection = false;
}
