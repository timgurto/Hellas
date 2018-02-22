#pragma once

#include <string>
#include <unordered_map>

#include "Renderer.h"
#include "Texture.h"

extern Renderer renderer;

class Images {
public:
    Images(const std::string &directory);

    const Texture &operator[](const std::string key);

private:
    using container = std::unordered_map<std::string, Texture>;
    container _container;
    std::string _directory;

    static Texture defaultTexture;
};
