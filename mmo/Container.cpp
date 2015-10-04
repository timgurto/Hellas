// (C) 2015 Tim Gurto

#include "Container.h"

Container::Container(size_t rows, size_t cols, containerVec_t &linked, int x, int y):
Element(makeRect(x, y, rows * Client::ICON_SIZE, cols * Client::ICON_SIZE)),
_rows(rows),
_cols(cols),
_linked(linked){}
