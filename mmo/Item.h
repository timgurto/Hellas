#ifndef ITEM_H
#define ITEM_H

#include <SDL.h>
#include <string>

// Describes an item type
class Item{
    std::string _id; // The no-space, unique name used in data files
    std::string _name;
    SDL_Surface *_icon;

public:
    Item(const std::string &id, const std::string &name);
    Item(const std::string &id); // Creates a dummy Item for set lookup
    ~Item();

    SDL_Surface *getIcon();

    bool operator<(const Item &rhs) const; // Compares ids
};

#endif
