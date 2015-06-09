#include "Branch.h"
#include "Color.h"
#include "util.h"

int Branch::currentSerial = 0;
int Branch::numBranches = 0;
SDL_Surface *Branch::image = 0;


Branch::Branch(const Branch &rhs):
location(rhs.location),
serial(rhs.serial){
    ++numBranches;
}

Branch::Branch(const Point &loc):
location(loc),
serial(currentSerial++){
    ++numBranches;
}

Branch::Branch(int serialArg, const Point &loc):
serial(serialArg),
location(loc){
    ++numBranches;
    if (!image) {
        image = SDL_LoadBMP("Images/branch.bmp");
        SDL_SetColorKey(image, SDL_TRUE, Color::MAGENTA);
    }
}

Branch::~Branch(){
    --numBranches;
    if (numBranches == 0)
        SDL_FreeSurface(image);
}

bool Branch::operator<(const Branch &rhs) const{
    return serial < rhs.serial;
}

void Branch::draw(SDL_Surface *dstSurface) const{
    if (image)
        SDL_BlitSurface(image, 0, dstSurface, &makeRect(location));
}
