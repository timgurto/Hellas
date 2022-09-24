#pragma once

#include <string>
#include <unordered_map>

#include "../Optional.h"
#include "Renderer.h"
#include "Texture.h"

class MemoisedImageDirectory {
 public:
  MemoisedImageDirectory() {}
  MemoisedImageDirectory(Color colorKey);
  void initialise(const std::string &directory);

  const Texture &operator[](const std::string key);
  static bool doesTextureExistInDirectory(const Texture &texture);

 private:
  using container = std::unordered_map<std::string, Texture>;
  container _container;
  std::string _directory;
  Optional<Color> _colorKey;

  static Texture defaultTexture;
  static void createDefaultTexture();
};
