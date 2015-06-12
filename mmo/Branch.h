#ifndef BRANCH_H
#define BRANCH_H

#include <SDL.h>
#include <sstream>
#include <string>

#include "Point.h"

// Describes a tree branch which can be collected by a user
class Branch{
    static int _currentSerial;
    static int _numBranches;
    static SDL_Surface *_image;

    int _serial;
    Point _location;

public:

    Branch(const Branch &rhs);
    Branch(const Point &loc); // Generates new serial; should only be called by server
    Branch(int serial, const Point &loc = 0); // No location: create dummy Branch, for set searches
    ~Branch();

    bool operator<(const Branch &rhs) const; // Compare serials

    friend std::ostream &operator<<(std::ostream &lhs, const Branch &rhs);

    int serial() const;
    const Point &location() const;
    
    void draw(SDL_Surface *dstSurface) const;

private:
};

#endif
