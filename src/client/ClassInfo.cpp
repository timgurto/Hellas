#include "ClassInfo.h"

ClassInfo::ClassInfo(const ID &id, const Name &name) : _name(name) {
    _image = { "Images/Humans/" + id + ".png", Color::MAGENTA };
}
