#ifndef SPRITE_TYPE_H
#define SPRITE_TYPE_H

#include <string>

#include "../Optional.h"
#include "../Point.h"
#include "../util.h"
#include "ImageWithHighlight.h"
#include "Texture.h"

class Client;

// Describes a class of Sprites, the "instances" of which share common
// properties
class SpriteType {
  ImageWithHighlight _image;
  mutable Texture _shadow;
  ScreenRect _drawRect;  // Where to draw the image, relative to its location
  bool _isFlat{false};   // Whether these objects appear flat, i.e., are drawn
                         // behind all other entities.
  bool _isDecoration{false};  // If true, cannot be interacted with.

  static ms_t timeThatTheLastRedrawWasOrdered;
  mutable ms_t _timeShadowGenerated{0};

 protected:
  static ms_t timeLastRedrawWasOrdered() {
    return timeThatTheLastRedrawWasOrdered;
  }

 private:
  struct Particles {
    std::string profile;
    MapPoint offset;  // From location
  };
  std::vector<Particles> _particles;

  Optional<px_t> _customShadowWidth;
  Optional<px_t>
      _customDrawHeight;  // If the image file has blank space at the bottom
  Optional<double> _customCullDistance;

 public:
  static const double SHADOW_RATIO, SHADOW_WIDTH_HEIGHT_RATIO;

  SpriteType(const ScreenRect &drawRect = {},
             const std::string &imageFile = "");
  static SpriteType DecorationWithNoData();

  virtual ~SpriteType() {}

  virtual void setImage(const std::string &filename);
  virtual const Texture &image() const { return _image.getNormalImage(); }
  const Texture &getHighlightImage() const {
    return _image.getHighlightImage();
  }
  const ImageWithHighlight &imageWithHighlight() const { return _image; }
  const ScreenRect &drawRect() const { return _drawRect; }
  void drawRect(const ScreenRect &rect);
  const Texture &shadow() const;
  bool isFlat() const { return _isFlat; }
  void isFlat(bool b) { _isFlat = b; }
  bool isDecoration() const { return _isDecoration; }
  void isDecoration(bool b) { _isDecoration = b; }
  px_t width() const { return _drawRect.w; }
  px_t height() const { return _drawRect.h; }
  void addParticles(const std::string &profileName, const MapPoint &offset);
  const std::vector<Particles> &particles() const { return _particles; }

  void setAlpha(Uint8 alpha) { _image.setAlpha(alpha); }

  virtual char classTag() const { return 'e'; }

  static void forceAllShadowsToRedraw() {
    timeThatTheLastRedrawWasOrdered = SDL_GetTicks();
  }
  void useCustomShadowWidth(px_t width) { _customShadowWidth = width; }
  bool hasCustomShadowWidth() const { return _customShadowWidth.hasValue(); }
  px_t customShadowWidth() const { return _customShadowWidth.value(); }
  void useCustomDrawHeight(px_t height) { _customDrawHeight = height; }
  bool hasCustomDrawHeight() const { return _customDrawHeight.hasValue(); }
  px_t customDrawHeight() const { return _customDrawHeight.value(); }
  void useCustomCullDistance(double dist) { _customCullDistance = dist; }
  bool hasCustomCullDistance() const { return _customCullDistance.hasValue(); }
  double customCullDistance() const { return _customCullDistance.value(); }
};

#endif
