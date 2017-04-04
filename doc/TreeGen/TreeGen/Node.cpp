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

Node::Node(NodeType type, const ID &id, const NiceName &niceName):
    type(type),
    id(id),
    name(typePrefix(type) + "_" + id),
    niceName(niceName)
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
        std::string imagePath = "../../Images/";
        std::string shape;
        std::string url;
        if (type == ITEM){
            url = "item.html?id=" + id;
            imagePath += "Items/" + id;
            shape = "none";
        }else if (type == NPC){
            url = "npc.html?id=" + id;
            imagePath += "NPCs/" + id;
            shape = "box";
        }else{
            url = "object.html?id=" + id;
            imagePath += "objects/" + id;
            shape = "box";
        }

        imagePath += ".png";

        // Generate image
        SDL_Surface *surface = IMG_Load(imagePath.c_str());
        bool imageExists = surface != nullptr;

        if (imageExists){
            SDL_SetColorKey(surface, SDL_TRUE, 0xff00ff); // Make magenta transparent

        Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
#else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
#endif

            // Stretch onto canvas
            static const size_t SCALAR = 2;
            SDL_Surface *canvas = SDL_CreateRGBSurface(0, surface->w * SCALAR, surface->h * SCALAR, 32, rmask, gmask, bmask, amask);
            SDL_Rect rect = {0, 0, surface->w * SCALAR, surface->h * SCALAR };
            SDL_BlitScaled(surface, nullptr, canvas, &rect);

            // Save new image
            IMG_SavePNG(canvas, ("../web/images/" + name + ".png").c_str());
        }

        std::string
            image = "<img src=\"../web/images/" + (imageExists ? name : "error") + ".png\"/>",
            label = std::string()
                +"<<table border='0' cellborder='0'>"
                    + "<tr><td>" + image + "</td></tr>"
                    + "<tr><td>" + niceName + "</td></tr>"
                + "</table>>",
            fullNode = name + " ["
                    + " shape=" + shape
                    + " tooltip=\"" + name + "\""
                    + " label=" + label
                    + " URL=\"" + url
                + "\"]";
        output << fullNode << std::endl;
}
