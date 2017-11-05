#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

#include "Node.h"

const std::string &Node::typePrefix(NodeType type){
    static std::map<NodeType, std::string> prefixDict;
    if (prefixDict.empty()){
        prefixDict[ITEM] = "item";
        prefixDict[OBJECT] = "object";
        prefixDict[NPC] = "npc";
    }
    return prefixDict[type];
}

Node::Node(NodeType type, const ID &id, const DisplayName &displayName):
    type(type),
    id(id),
    name(typePrefix(type) + "_" + id),
    image(id),
    displayName(displayName),
    imageExists(false)
    {}

Node::Node(Name name):
    name(name)
{}

bool Node::operator==(const Name &rhs) const{
    return name == rhs;
}

bool Node::operator<(const Node &rhs) const{
    return name < rhs.name;
}

void Nodes::add(const Node &node){
    set.insert(node);
}

void Nodes::remove(const Node::Name &name){
    auto it = set.find(Node::dummy(name));
    if (it == set.end()){
        std::cout << "Can't find node to remove: " << name << std::endl;
        return;
    }
    set.erase(it);
}
    
void Nodes::outputAsGraphviz(std::ostream &output) const{
    for (const Node &node : set)
        node.outputAsGraphviz(output);
}

void Node::outputAsGraphviz(std::ostream &output) const{
        std::string shape;
        std::string url;
        if (type == ITEM){
            url = "item.html?id=" + id;
            shape = "none";
        }else if (type == NPC){
            url = "npc.html?id=" + id;
            shape = "box";
        }else{
            url = "object.html?id=" + id;
            shape = "box";
        }

        std::string
            imageFile = typePrefix(type) + "_" + image,
            imagePart = "<img src=\"../web/images/" + (imageExists ? imageFile : "error") + ".png\"/>",
            labelPart = std::string()
                +"<<table border='0' cellborder='0'>"
                    + "<tr><td>" + imagePart + "</td></tr>"
                    + "<tr><td>" + displayName + "</td></tr>"
                + "</table>>",
            fullNode = name + " ["
                    + " shape=" + shape
                    + " tooltip=\"" + name + "\""
                    + " label=" + labelPart
                    + " URL=\"" + url
                + "\"]";
        output << fullNode << std::endl;
}

void Nodes::generateAllImages() const{
    for (const Node &node : set)
        node.generateImage();
}

void Node::generateImage() const{
    std::string imagePath = "../../Images/";
    if (type == ITEM){
        imagePath += "Items/" + image;
    }else if (type == NPC){
        imagePath += "NPCs/" + image;
    }else{
        imagePath += "objects/" + image;
    }

    imagePath += ".png";

    // Generate image
    std::cout << "Loading image " << imagePath << std::endl;
    SDL_Surface *surface = IMG_Load(imagePath.c_str());
    imageExists = surface != nullptr;

    if (!imageExists)
        return;

    if (type == ITEM)
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE); // Image has alpha channel
    else
        SDL_SetColorKey(surface, SDL_TRUE, 0xff00ff); // Make magenta transparent

    static const Uint32
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000,
        gmask = 0x00ff0000,
        bmask = 0x0000ff00,
        amask = 0x000000ff;
#else
        rmask = 0x000000ff,
        gmask = 0x0000ff00,
        bmask = 0x00ff0000,
        amask = 0xff000000;
#endif

    // Stretch onto canvas
    static const int SCALAR = 2;
    SDL_Surface *canvas = SDL_CreateRGBSurface(0, surface->w * SCALAR, surface->h * SCALAR, 32, rmask, gmask, bmask, amask);
    SDL_Rect rect = {0, 0, surface->w * SCALAR, surface->h * SCALAR };
    SDL_BlitScaled(surface, nullptr, canvas, &rect);

    // Save new image
    std::string outputImageFileName = typePrefix(type) + "_" + image;
    IMG_SavePNG(canvas, ("../web/images/" + outputImageFileName + ".png").c_str());
}
