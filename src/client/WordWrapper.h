#pragma once

#include <vector>
#include <string>

#include "../types.h"

class WordWrapper {
public:
    using Lines = std::vector<std::string>;

    WordWrapper(TTF_Font *font, px_t width);

    Lines wrap(const std::string &unwrapped) const;

private:
    px_t _width;
};
