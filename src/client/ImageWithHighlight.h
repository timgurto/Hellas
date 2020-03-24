#pragma once

#include "Texture.h"

// This class is provided with an image, and can then be used to draw either the
// base image or the image with a bright outline around its edge.
// The highlighted image is generated only on first use.
class ImageWithHighlight {
 public:
  ImageWithHighlight(const FilenameWithoutSuffix& imageFile);
  ImageWithHighlight(){};

  const Texture& getNormalImage() const { return _image; }
  const Texture& getHighlightImage() const;

  px_t width() const { return _image.width(); }
  px_t height() const { return _image.height(); }

  void setAlpha(Uint8 alpha) { _image.setAlpha(alpha); }
  void rotateClockwise(const ScreenPoint& centre);

  static void forceAllToRedraw();

 private:
  void redrawHighlightImage() const;

  Texture _image;
  mutable Texture _highlightImage;
  Filename _imageFile;

  mutable ms_t _timeHighlightGenerated{0};
  static ms_t timeThatTheLastRedrawWasOrdered;
};
