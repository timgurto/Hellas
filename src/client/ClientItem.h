#ifndef CLIENT_ITEM_H
#define CLIENT_ITEM_H

#include <map>

#include "Texture.h"
#include "../Item.h"

class ClientObjectType;
struct Mix_Chunk;

// The client-side representation of an item type
class ClientItem : public Item{
    std::string _name;
    Texture _icon;
    Texture _gearImage;
    Point _drawLoc;
    mutable Texture _tooltip;
    Mix_Chunk *_dropSound; // When dropping the item in a container

    // The object that this item can construct
    const ClientObjectType *_constructsObject;

    static std::map<int, size_t> gearDrawOrder;
    static std::vector<Point> gearOffsets;

public:
    ClientItem(const std::string &id, const std::string &name = "");

    const std::string &name() const { return _name; }
    const Texture &icon() const { return _icon; }
    void icon(const std::string &filename);
    void gearImage(const std::string &filename);
    void drawLoc(const Point &loc) { _drawLoc = loc; }
    static const std::map<int, size_t> &drawOrder() { return gearDrawOrder; }
    void dropSound(const std::string &filename);
    Mix_Chunk *dropSound() const { return _dropSound; }

    typedef std::vector<std::pair<const ClientItem *, size_t> > vect_t;

    void constructsObject(const ClientObjectType *obj) { _constructsObject = obj; }
    const ClientObjectType *constructsObject() const { return _constructsObject; }

    const Texture &tooltip() const; // Getter; creates tooltip on first call.

    void draw(const Point &loc) const;
    void playDropSound() const;

    static void init();
};

const ClientItem *toClientItem(const Item *item);

#endif
