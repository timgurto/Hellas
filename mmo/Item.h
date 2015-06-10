#ifndef ITEM_H
#define ITEM_H

#include <SDL.h>
#include <string>

// Describes an item type
struct Item{
    std::string id; // The no-space, unique name used in data files
    std::string name;
    size_t stackSize;
    SDL_Surface *icon;

    Item(const std::string &id, const std::string &name, size_t stackSize = 1);
    Item(const std::string &id); // Creates a dummy Item for set lookup
    ~Item();

    SDL_Surface *getIcon();

    bool operator<(const Item &rhs) const; // Compares ids
};

#endif
