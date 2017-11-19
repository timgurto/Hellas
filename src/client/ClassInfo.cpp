#include "ClassInfo.h"

ClassInfo::ClassInfo(const Name &name) : _name(name) {
    _image = { "Images/Humans/" + name + ".png", Color::MAGENTA };
}
