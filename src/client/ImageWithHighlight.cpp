#include "ImageWithHighlight.h"

#include "Renderer.h"

extern Renderer renderer;

ms_t ImageWithHighlight::timeThatTheLastRedrawWasOrdered{0};

ImageWithHighlight::ImageWithHighlight(const FilenameWithoutSuffix& imageFile) {
  if (imageFile.empty()) return;
  _imageFile = imageFile + ".png";
  if (!renderer) return;
  _image = Texture(_imageFile, Color::MAGENTA);
}

const Texture& ImageWithHighlight::getHighlightImage() const {
  auto isUpToDate = _highlightImage &&
                    _timeHighlightGenerated >= timeThatTheLastRedrawWasOrdered;
  if (!isUpToDate) redrawHighlightImage();

  return _highlightImage;
}

void ImageWithHighlight::rotateClockwise(const ScreenPoint& centre) {
  _image.rotateClockwise(centre);
  redrawHighlightImage();
}

void ImageWithHighlight::forceAllToRedraw() {
  timeThatTheLastRedrawWasOrdered = SDL_GetTicks();
}

void ImageWithHighlight::redrawHighlightImage() const {
  _timeHighlightGenerated = SDL_GetTicks();

  auto surface = Surface{_imageFile, Color::MAGENTA};
  if (!surface) return;

  const Uint32 ALPHA_FRIENDLY_HIGHLIGHT_COLOUR =
      Uint32{Color::SPRITE_OUTLINE_HIGHLIGHT} | 0xff000000;

  surface.swapAllVisibleColors(ALPHA_FRIENDLY_HIGHLIGHT_COLOUR);
  auto singleRecolor = Texture{surface};
  singleRecolor.setAlpha(0x9f);

  _highlightImage = Texture{_image.width() + 2, _image.height() + 2};
  _highlightImage.setBlend();
  renderer.pushRenderTarget(_highlightImage);
  for (auto x = 0; x <= 2; ++x)
    for (auto y = 0; y <= 2; ++y) {
      if (x == 1 && y == 1) continue;
      singleRecolor.draw(x, y);
    }
  _image.draw(1, 1);
  renderer.popRenderTarget();
}
