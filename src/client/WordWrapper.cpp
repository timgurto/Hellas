#include "WordWrapper.h"

WordWrapper::WordWrapper(TTF_Font *font, px_t width) :
    _width(width) {
    }
}

WordWrapper::Lines WordWrapper::wrap(const std::string & unwrapped) const {
    return{ unwrapped };
}
