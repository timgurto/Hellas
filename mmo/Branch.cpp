#include "Branch.h"
#include "Color.h"
#include "util.h"

int Branch::_currentSerial = 0;
int Branch::_numBranches = 0;
SDL_Surface *Branch::_image = 0;


Branch::Branch(const Branch &rhs):
_location(rhs._location),
_serial(rhs._serial){
    ++_numBranches;
}

Branch::Branch(const Point &loc):
_location(loc),
_serial(_currentSerial++){
    ++_numBranches;
}

Branch::Branch(int serialArg, const Point &loc):
_serial(serialArg),
_location(loc){
    ++_numBranches;
    if (!_image) {
        _image = SDL_LoadBMP("Images/branch.bmp");
        SDL_SetColorKey(_image, SDL_TRUE, Color::MAGENTA);
    }
}

Branch::~Branch(){
    --_numBranches;
    if (_numBranches == 0)
        SDL_FreeSurface(_image);
}

bool Branch::operator<(const Branch &rhs) const{
    return _serial < rhs._serial;
}

int Branch::serial() const{
    return _serial;
}

const Point &Branch::location() const{
    return _location;
}

void Branch::draw(SDL_Surface *dstSurface) const{
    if (_image)
        SDL_BlitSurface(_image, 0, dstSurface, &makeRect(_location));
}
