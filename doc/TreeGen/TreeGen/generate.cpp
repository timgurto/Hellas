#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <queue>
#include <XmlReader.h>
#include <SDL.h>
#include <SDL_image.h>

#include "JsonWriter.h"

struct Path{
    std::string child;
    std::vector<std::string> parents;

    Path(const std::string &startNode):
        child(startNode)
    {
        parents.push_back(startNode);
    }
};

enum EdgeType{
    GATHER,
    CONSTRUCT_FROM_ITEM,
    GATHER_REQ,
    CONSTRUCTION_REQ,
    LOOT,
    INGREDIENT,
    TRANSFORM,

    UNLOCK_ON_GATHER,
    UNLOCK_ON_ACQUIRE,
    UNLOCK_ON_CRAFT,
    UNLOCK_ON_CONSTRUCT,

    DEFAULT
};

struct Edge{
    std::string parent, child;
    EdgeType type;
    double chance;
    
    Edge(const std::string &from, const std::string &to, EdgeType typeArg, double chanceArg = 1.0): parent(from), child(to), type(typeArg), chance(chanceArg) {}
    bool operator==(const Edge &rhs) const { return parent == rhs.parent && child == rhs.child; }
    bool operator<(const Edge &rhs) const{
        if (parent != rhs.parent) return parent < rhs.parent;
        return child < rhs.child;
    }
};

#undef main
int main(int argc, char **argv){
    std::set<Edge> edges;
    std::map<std::string, std::string> tools;
    std::map<std::string, std::string> nodes; // id -> label
    std::set<Edge > blacklist, extras;

    std::string colorScheme = "rdylgn6";
    std::map<EdgeType, size_t> edgeColors;
    // Weak: light colors
    edgeColors[UNLOCK_ON_ACQUIRE] = 1; // Weak, since you can trade for the item.
    edgeColors[GATHER] = 1;
    edgeColors[CONSTRUCT_FROM_ITEM] = 2;
    edgeColors[UNLOCK_ON_GATHER] = 3;
    edgeColors[LOOT] = 4;
    edgeColors[GATHER_REQ] = 5;
    edgeColors[CONSTRUCTION_REQ] = 5;
    // Strong: dark colors
    edgeColors[TRANSFORM] = 6;
    edgeColors[UNLOCK_ON_CONSTRUCT] = 6;
    edgeColors[UNLOCK_ON_CRAFT] = 6;

    const std::string dataPath = "../../Data";

    // Load tools
    XmlReader xr("archetypalTools.xml");
    for (auto elem : xr.getChildren("tool")){
        std::string id, archetype;
        if (!xr.findAttr(elem, "id", id)) continue;
        if (!xr.findAttr(elem, "archetype", archetype)) continue;
        tools[id] = archetype;
    }

    // Load blacklist
    for (auto elem : xr.getChildren("blacklist")){
        std::string parent, child;
        if (!xr.findAttr(elem, "parent", parent)){
            std::cout << "Blacklist item had no parent; ignored" << std::endl;
            continue;
        }
        if (!xr.findAttr(elem, "child", child)){
            std::cout << "Blacklist item had no child; ignored" << std::endl;
            continue;
        }
        blacklist.insert(Edge(parent, child, DEFAULT));
    }

    // Load extra edges
    for (auto elem : xr.getChildren("extra")){
        std::string parent, child;
        if (!xr.findAttr(elem, "parent", parent)){
            std::cout << "Blacklist item had no parent; ignored" << std::endl;
            continue;
        }
        if (!xr.findAttr(elem, "child", child)){
            std::cout << "Blacklist item had no child; ignored" << std::endl;
            continue;
        }
        Edge e(parent, child, DEFAULT);
        extras.insert(e);
    }

    // Load items
    if (!xr.newFile(dataPath + "/items.xml"))
        std::cerr << "Failed to load items.xml" << std::endl;
    else{
        JsonWriter jw("items");
        for (auto elem : xr.getChildren("item")) {
            jw.nextEntry();
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            std::string label = "item_" + id;
            jw.addAttribute("image", label);
            jw.addAttribute("id", id);

            std::string s;
            if (xr.findAttr(elem, "name", s)){
                nodes.insert(std::make_pair(label, s));
                jw.addAttribute("name", s);
            }

            if (xr.findAttr(elem, "constructs", s)){
                edges.insert(Edge(label, "object_" + s, CONSTRUCT_FROM_ITEM));
                jw.addAttribute("constructs", s);
            }
        }
    }

    // Load objects
    if (!xr.newFile(dataPath + "/objectTypes.xml"))
        std::cerr << "Failed to load objectTypes.xml" << std::endl;
    else{
        JsonWriter jw("objects");
        for (auto elem : xr.getChildren("objectType")) {
            jw.nextEntry();
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            std::string label = "object_" + id;
            jw.addAttribute("image", label);
            jw.addAttribute("id", id);

            std::string s;
            if (xr.findAttr(elem, "name", s)){
                nodes.insert(std::make_pair(label, s));
                jw.addAttribute("name", s);
            }

            if (xr.findAttr(elem, "gatherReq", s)){
                auto it = tools.find(s);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << s << std::endl;
                    edges.insert(Edge(s, label, GATHER_REQ));
                } else
                    edges.insert(Edge(it->second, label, GATHER_REQ));
                jw.addAttribute("gatherReq", s);
            }

            if (xr.findAttr(elem, "constructionReq", s)){
                auto it = tools.find(s);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << s << std::endl;
                    edges.insert(Edge(s, label, CONSTRUCTION_REQ));
                } else
                    edges.insert(Edge(it->second, label, CONSTRUCTION_REQ));
                jw.addAttribute("constructionReq", s);
            }

            std::set<std::string> yields;
            for (auto yield : xr.getChildren("yield", elem)) {
                if (!xr.findAttr(yield, "id", s))
                    continue;
                edges.insert(Edge(label, "item_" + s, GATHER));
                yields.insert(s);
            }
            jw.addArrayAttribute("yield", yields);

            std::set<std::string> unlocksForJson;
            for (auto unlockBy : xr.getChildren("unlockedBy", elem)){
                double chance = 1.0;
                xr.findAttr(unlockBy, "chance", chance);
                if (xr.findAttr(unlockBy, "recipe", s) || xr.findAttr(unlockBy, "item", s)){
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_CRAFT, chance));
                    unlocksForJson.insert("{type:\"craft\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "construction", s)){
                    edges.insert(Edge("object_" + s, label, UNLOCK_ON_CONSTRUCT, chance));
                    unlocksForJson.insert("{type:\"construct\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "gather", s)){
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_GATHER, chance));
                    unlocksForJson.insert("{type:\"gather\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "item", s)){
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_ACQUIRE, chance));
                    unlocksForJson.insert("{type:\"acquire\", sourceID:\"" + s + "\"}");
                }
            }
            jw.addArrayAttribute("unlockedBy", unlocksForJson, true);

            auto transform = xr.findChild("transform", elem);
            if (transform && xr.findAttr(transform, "id", s)){
                edges.insert(Edge(label, "object_" + s, TRANSFORM));
                jw.addAttribute("transformID", s);
                if (xr.findAttr(transform, "time", s)) jw.addAttribute("transformTime", s);
            }
            
            if (xr.findAttr(elem, "gatherTime", s)) jw.addAttribute("gatherTime", s);
            if (xr.findAttr(elem, "constructionTime", s)) jw.addAttribute("constructionTime", s);
        }
    }

    // Load NPCs
    if (!xr.newFile(dataPath + "/npcTypes.xml"))
        std::cerr << "Failed to load npcTypes.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("npcType")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            std::string label = "npc_" + id;

            std::string s;
            if (xr.findAttr(elem, "name", s))
                nodes.insert(std::make_pair(label, s));

            for (auto loot : xr.getChildren("loot", elem)) {
                if (!xr.findAttr(loot, "id", s))
                    continue;
                edges.insert(Edge(label, "item_" + s, LOOT));
            }
        }
    }


    // Load recipes
    if (!xr.newFile(dataPath + "/recipes.xml"))
        std::cerr << "Failed to load recipes.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("recipe")) {
            std::string product;
            if (!xr.findAttr(elem, "product", product))
                continue;
            std::string label = "item_" + product;

            std::string s;

            for (auto unlockBy : xr.getChildren("unlockedBy", elem)){
                double chance = 1.0;
                xr.findAttr(unlockBy, "chance", chance);
                if (xr.findAttr(unlockBy, "recipe", s) || xr.findAttr(unlockBy, "item", s))
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_CRAFT, chance));
                else if (xr.findAttr(unlockBy, "construction", s))
                    edges.insert(Edge("object_" + s, label, UNLOCK_ON_CONSTRUCT, chance));
                else if (xr.findAttr(unlockBy, "gather", s))
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_GATHER, chance));
                else if (xr.findAttr(unlockBy, "item", s))
                    edges.insert(Edge("item_" + s, label, UNLOCK_ON_ACQUIRE, chance));
            }
        }
    }

    // Remove blacklisted items
    for (auto blacklistedEdge : blacklist)
        for (auto it = edges.begin(); it != edges.end(); ++it)
            if (*it == blacklistedEdge){
                edges.erase(it);
                break;
            }


    // Remove shortcuts
    for (auto edgeIt = edges.begin(); edgeIt != edges.end(); ){
        /*
        This is one edge.  We want to figure out if there's a longer path here
        (i.e., this path doesn't add any new information about progress requirements).
        */
        bool shouldDelete = false;
        std::queue<std::string> queue;
        std::set<std::string> nodesFound;

        // Populate queue initially with all direct children
        for (const Edge &edge : edges)
            if (edge.parent == edgeIt->parent && edge.child != edgeIt->child)
                queue.push(edge.child);

        // New nodes to check will be added here.  Effectively it will be a breadth-first search.
        while (!queue.empty() && !shouldDelete){
            std::string nextParent = queue.front();
            queue.pop();
            for (const Edge &edge : edges){
                if (edge.parent != nextParent)
                continue;
                if (edge.child == edgeIt->child){
                    // Mark the edge for removal
                    shouldDelete = true;
                    break;
                } else if (nodesFound.find(edge.child) != nodesFound.end()){
                    queue.push(edge.child);
                    nodesFound.insert(edge.child);
                }
            }
        }
        auto nextIt = edgeIt; ++nextIt;
        if (shouldDelete)
            edges.erase(edgeIt);
        edgeIt = nextIt;
    }


    // Remove loops
    // First, find set of nodes that start edges but don't finish them (i.e., the roots of the tree).
    std::set<std::string> starts, ends;
    for (auto &edge : edges){
        starts.insert(edge.parent);
        ends.insert(edge.child);
    }
    for (const std::string &endNode : ends){
        starts.erase(endNode);
    }

    // Next, do a BFS from each to find loops.  Remove those final edges.
    std::set<Edge > trashCan;
    for (const std::string &startNode : starts){
        //std::cout << "Root node: " << startNode << std::endl;
        std::queue<Path> queue;
        queue.push(Path(startNode));
        bool loopFound = false;
        while (!queue.empty() && !loopFound){
            Path nextPath = queue.front();
            queue.pop();
            for (auto it = edges.begin(); it != edges.end(); ++it){
                if (it->parent != nextPath.child)
                    continue;
                std::string child = it->child;
                // If this child is already a parent
                if (std::find(nextPath.parents.begin(), nextPath.parents.end(), child) != nextPath.parents.end()){
                    std::cout << "Loop found; marked for removal:" << std::endl << "  ";
                    for (const std::string &parent : nextPath.parents)
                        std::cout << parent << " -> ";
                    std::cout << child << std::endl;
                    // Mark the edge for removal
                    trashCan.insert(*it);
                } else {
                    Path p = nextPath;
                    p.parents.push_back(child);
                    p.child = child;
                    queue.push(p);
                }
            }
        }
        for (auto edgeToDelete : trashCan)
            for (auto it = edges.begin(); it != edges.end(); ++it)
                if (*it == edgeToDelete){
                    edges.erase(edgeToDelete);
                    break;
                }
    }


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


    // Publish
    std::ofstream f("tree.gv");
    f << "digraph techTree {" << std::endl;
    f << "bgcolor=\"#ffffff00\"" << std::endl;
    f << "node [fontsize=10 fontname=\"Advocut\" imagescale=true fontcolor=\"#999999\" color=\"#999999\"];" << std::endl;
    f << "edge [fontsize=10 fontname=\"Advocut\" fontcolor=\"#999999\" color=\"#999999\"];" << std::endl;

    // Nodes
    for (auto &node : nodes){
        std::string imagePath = "../../Images/";
        std::string shape;
        std::string url;
        if (node.first.substr(0, 5) == "item_"){
            std::string id = node.first.substr(5);
            url = "item.html?id=" + id;
            imagePath += "Items/" + id;
            shape = "none";
        }else if (node.first.substr(0, 4) == "npc_"){
            std::string id = node.first.substr(4);
            url = "npc.html?id=" + id;
            imagePath += "NPCs/" + id;
            shape = "box";
        }else{
            std::string id = node.first.substr(7);
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

            // Stretch onto canvas
            static const size_t SCALAR = 2;
            SDL_Surface *canvas = SDL_CreateRGBSurface(0, surface->w * SCALAR, surface->h * SCALAR, 32, rmask, gmask, bmask, amask);
            SDL_Rect rect = {0, 0, surface->w * SCALAR, surface->h * SCALAR };
            SDL_BlitScaled(surface, nullptr, canvas, &rect);

            // Save new image
            IMG_SavePNG(canvas, ("../web/images/" + node.first + ".png").c_str());
        }

        std::string
            id = node.first,
            name = node.second,
            image = "<img src=\"../web/images/" + (imageExists ? node.first : "error") + ".png\"/>",
            fullNode = node.first + " [shape=" + shape
                    + " tooltip=\"" + name + "\""
                    + " label=<<table border='0' cellborder='0'>"
                    + "<tr><td>" + image + "</td></tr>"
                    + "<tr><td>" + name + "</td></tr>"
                    + "</table>> URL=\"" + url + "\"]";
        f << fullNode << std::endl;
    }

    for (auto &edge : edges){
        f << edge.parent << " -> " << edge.child
          << " [colorscheme=\"" << colorScheme << "\", color=" << edgeColors[edge.type];
        if (edge.chance < 1.0)
            f << " label=\"" << static_cast<int>(edge.chance*100 + 0.5) << "%\"";
       f << "]" << std::endl;
    }

    // Extra edges
    for (auto &edge : extras)
        f << edge.parent << " -> " << edge.child
          << " [style=dotted constraint=true]"
          << std::endl;

    f << "}";

    return 0;
}
