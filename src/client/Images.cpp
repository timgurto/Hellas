#include "Images.h"

Texture Images::defaultTexture;

Images::Images(const std::string &directory) : _directory(directory) {
  // Initialize default texture
  if (defaultTexture) return;
  defaultTexture = {16, 16};

  defaultTexture.setRenderTarget();

  renderer.setDrawColor(Color::TODO);
  renderer.fill();

  renderer.setRenderTarget();
}

const Texture &Images::operator[](const std::string key) {
  auto it = _container.find(key);

  // Image exists: return it
  if (it != _container.end()) return it->second;

  // Doesn't exist: load it
  auto tex = Texture{_directory + "/" + key + ".png"};
  if (tex) {
    _container[key] = tex;
    return _container[key];
  }

  // File doesn't exist: return default
  return defaultTexture;
}
