#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <queue>
#include <XmlReader.h>

#include "Edge.h"
#include "Node.h"
#include "JsonWriter.h"
#include "types.h"

struct Path{
    std::string child;
    std::vector<std::string> parents;

    Path(const std::string &startNode):
        child(startNode)
    {
        parents.push_back(startNode);
    }
};

#undef main
int main(int argc, char **argv){
    std::set<Edge> edges;
    std::map<Tag, Node::Name> tools;
    Nodes nodes;
    std::set<Edge > blacklist, extras;
    std::map<std::string, std::string> collapses;

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
        std::string archetype;
        Tag tag;
        if (!xr.findAttr(elem, "id", tag)) continue;
        if (!xr.findAttr(elem, "archetype", archetype)) continue;
        tools[tag] = archetype;
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

    // Load collapses
    for (auto elem : xr.getChildren("collapse")){
        std::string id, into;
        if (!xr.findAttr(elem, "id", id)){
            std::cout << "Collapse item had no source; ignored" << std::endl;
            continue;
        }
        if (!xr.findAttr(elem, "into", into)){
            std::cout << "Collapse item had no target; ignored" << std::endl;
            continue;
        }
        collapses[id] = into;
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
            ID id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            Node::Name name = "item_" + id;
            jw.addAttribute("image", name);
            jw.addAttribute("id", id);

            std::string s;
            if (xr.findAttr(elem, "name", s)){
                nodes.add(Node(ITEM, id, s));
                jw.addAttribute("name", s);
            }

            if (xr.findAttr(elem, "constructs", s)){
                edges.insert(Edge(name, "object_" + s, CONSTRUCT_FROM_ITEM));
                jw.addAttribute("constructs", s);
            }

            std::set<std::string> tags;
            for (auto tag : xr.getChildren("tag", elem))
                if (xr.findAttr(tag, "name", s))
                    tags.insert(s);
            jw.addArrayAttribute("tags", tags);
            
            if (xr.findAttr(elem, "stackSize", s)) jw.addAttribute("stackSize", s);
            if (xr.findAttr(elem, "gearSlot", s)) jw.addAttribute("gearSlot", s);

            for (auto stat : xr.getChildren("stats", elem)) {
                if (xr.findAttr(stat, "health", s)) jw.addAttribute("health", s);
                if (xr.findAttr(stat, "attack", s)) jw.addAttribute("attack", s);
                if (xr.findAttr(stat, "speed", s)) jw.addAttribute("speed", s);
                if (xr.findAttr(stat, "attackTime", s)) jw.addAttribute("attackTime", s);
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
            ID id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            Node::Name name = "object_" + id;

            jw.addAttribute("image", name);
            jw.addAttribute("id", id);

            std::string s;
            if (xr.findAttr(elem, "name", s)){
                nodes.add(Node(OBJECT, id, s));
                jw.addAttribute("name", s);
            }

            ID gatherReq;
            if (xr.findAttr(elem, "gatherReq", gatherReq)){
                auto it = tools.find(gatherReq);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << gatherReq << std::endl;
                    edges.insert(Edge(gatherReq, name, GATHER_REQ));
                } else
                    edges.insert(Edge(it->second, name, GATHER_REQ));
                jw.addAttribute("gatherReq", s);
            }

            if (xr.findAttr(elem, "constructionReq", s)){
                auto it = tools.find(s);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << s << std::endl;
                    edges.insert(Edge(s, name, CONSTRUCTION_REQ));
                } else
                    edges.insert(Edge(it->second, name, CONSTRUCTION_REQ));
                jw.addAttribute("constructionReq", s);
            }

            std::set<std::string> yields;
            for (auto yield : xr.getChildren("yield", elem)) {
                if (!xr.findAttr(yield, "id", s))
                    continue;
                edges.insert(Edge(name, "item_" + s, GATHER));
                yields.insert(s);
            }
            jw.addArrayAttribute("yield", yields);

            std::set<std::string> materialsForJson;
            for (auto material : xr.getChildren("material", elem)) {
                std::string quantity = "1";
                if (!xr.findAttr(material, "id", s))
                    continue;
                xr.findAttr(material, "quantity", quantity); // Default = 1, above.
                materialsForJson.insert("{id:\"" + s + "\", quantity:" + quantity + "}");
            }
            jw.addArrayAttribute("materials", materialsForJson, true);

            std::set<std::string> unlocksForJson;
            for (auto unlockBy : xr.getChildren("unlockedBy", elem)){
                double chance = 1.0;
                xr.findAttr(unlockBy, "chance", chance);
                if (xr.findAttr(unlockBy, "recipe", s) || xr.findAttr(unlockBy, "item", s)){
                    edges.insert(Edge("item_" + s, name, UNLOCK_ON_CRAFT, chance));
                    unlocksForJson.insert("{type:\"craft\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "construction", s)){
                    edges.insert(Edge("object_" + s, name, UNLOCK_ON_CONSTRUCT, chance));
                    unlocksForJson.insert("{type:\"construct\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "gather", s)){
                    edges.insert(Edge("item_" + s, name, UNLOCK_ON_GATHER, chance));
                    unlocksForJson.insert("{type:\"gather\", sourceID:\"" + s + "\"}");
                } else if (xr.findAttr(unlockBy, "item", s)){
                    edges.insert(Edge("item_" + s, name, UNLOCK_ON_ACQUIRE, chance));
                    unlocksForJson.insert("{type:\"acquire\", sourceID:\"" + s + "\"}");
                }
            }
            jw.addArrayAttribute("unlockedBy", unlocksForJson, true);

            auto transform = xr.findChild("transform", elem);
            if (transform && xr.findAttr(transform, "id", s)){
                edges.insert(Edge(name, "object_" + s, TRANSFORM));
                jw.addAttribute("transformID", s);
                if (xr.findAttr(transform, "time", s)) jw.addAttribute("transformTime", s);
            }
            
            if (xr.findAttr(elem, "gatherTime", s)) jw.addAttribute("gatherTime", s);
            if (xr.findAttr(elem, "constructionTime", s)) jw.addAttribute("constructionTime", s);
            if (xr.findAttr(elem, "merchantSlots", s)) jw.addAttribute("merchantSlots", s);
            if (xr.findAttr(elem, "bottomlessMerchant", s) && s != "0") jw.addAttribute("bottomlessMerchant", "true");
            if (xr.findAttr(elem, "isVehicle", s) && s != "0") jw.addAttribute("isVehicle", "true");
            if (xr.findAttr(elem, "deconstructs", s)) jw.addAttribute("deconstructs", s);
            if (xr.findAttr(elem, "deconstructionTime", s)) jw.addAttribute("deconstructionTime", s);

            std::set<std::string> tags;
            for (auto tag : xr.getChildren("tag", elem))
                if (xr.findAttr(tag, "name", s))
                    tags.insert(s);
            jw.addArrayAttribute("tags", tags);

            auto container = xr.findChild("container", elem);
            if (container && xr.findAttr(container, "slots", s))
                jw.addAttribute("containerSlots", s);
        }
    }

    // Load NPCs
    if (!xr.newFile(dataPath + "/npcTypes.xml"))
        std::cerr << "Failed to load npcTypes.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("npcType")) {
            ID id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            Node::Name name = "npc_" + id;

            std::string s;
            if (xr.findAttr(elem, "name", s))
                nodes.add(Node(NPC, id, s));

            for (auto loot : xr.getChildren("loot", elem)) {
                if (!xr.findAttr(loot, "id", s))
                    continue;
                edges.insert(Edge(name, "item_" + s, LOOT));
            }
        }
    }


    // Load recipes
    if (!xr.newFile(dataPath + "/recipes.xml"))
        std::cerr << "Failed to load recipes.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("recipe")) {
            ID product;
            if (!xr.findAttr(elem, "product", product))
                continue;
            std::string label = "item_" + product;

            ID id;

            for (auto unlockBy : xr.getChildren("unlockedBy", elem)){
                double chance = 1.0;
                xr.findAttr(unlockBy, "chance", chance);
                if (xr.findAttr(unlockBy, "recipe", id) || xr.findAttr(unlockBy, "item", id))
                    edges.insert(Edge("item_" + id, label, UNLOCK_ON_CRAFT, chance));
                else if (xr.findAttr(unlockBy, "construction", id))
                    edges.insert(Edge("object_" + id, label, UNLOCK_ON_CONSTRUCT, chance));
                else if (xr.findAttr(unlockBy, "gather", id))
                    edges.insert(Edge("item_" + id, label, UNLOCK_ON_GATHER, chance));
                else if (xr.findAttr(unlockBy, "item", id))
                    edges.insert(Edge("item_" + id, label, UNLOCK_ON_ACQUIRE, chance));
            }
        }
    }

    // Collapse nodes
    {
        std::set<Edge> newEdges;
        for (auto it = edges.begin(); it != edges.end(); ){
            auto nextIt = it; ++nextIt;
            bool edgeNeedsReplacing = false;
            Edge newEdge = *it;
            if (collapses.find(it->parent) != collapses.end()){
                newEdge.parent = collapses[newEdge.parent];
                edgeNeedsReplacing = true;
            }
            if (collapses.find(it->child) != collapses.end()){
                newEdge.child = collapses[newEdge.child];
                edgeNeedsReplacing = true;
            }
            if (edgeNeedsReplacing){
                std::cout << "Collapsing edge:" << std::endl
                          << "            " << *it << std::endl
                          << "    becomes " << newEdge << std::endl;
                newEdges.insert(newEdge);
                edges.erase(it);
            }
            it = nextIt;
        }
        for (const Edge &newEdge : newEdges){
            edges.insert(newEdge);
        }
    }
    std::set<Node::Name> nodesToRemove;
    for (const auto &collapse : collapses)
        nodesToRemove.insert(collapse.first);
    for (const Node::Name &name : nodesToRemove)
        nodes.remove(name);

    // Remove blacklisted items
    for (auto blacklistedEdge : blacklist){
        for (auto it = edges.begin(); it != edges.end(); ++it)
            if (*it == blacklistedEdge){
                std::cout << "Removing blacklisted edge: " << *it << std::endl;
                edges.erase(it);
                break;
            }
    }


    // Remove self-references
    for (auto edgeIt = edges.begin(); edgeIt != edges.end(); ){
        auto nextIt = edgeIt; ++nextIt;
        if (edgeIt->parent == edgeIt->child){
            std::cout << "Removing self-referential edge " << *edgeIt << std::endl;
            edges.erase(edgeIt);
        }
        edgeIt = nextIt;
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
                    std::cout << "Removing shortcut: " << edge << std::endl;
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


    // Publish
    std::ofstream f("tree.gv");
    f << "digraph techTree {" << std::endl;
    f << "bgcolor=\"#ffffff00\"" << std::endl;
    f << "node [fontsize=10 fontname=\"Advocut\" imagescale=true fontcolor=\"#999999\" color=\"#999999\"];" << std::endl;
    f << "edge [fontsize=10 fontname=\"Advocut\" fontcolor=\"#999999\" color=\"#999999\"];" << std::endl;

    // Nodes
    nodes.outputAsGraphviz(f);

    // Edges
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
