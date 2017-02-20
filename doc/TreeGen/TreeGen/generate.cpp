#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <XmlReader.h>

int main(){
    std::multimap<std::string, std::string> map; // map[a] = b: a -> b
    std::map<std::string, std::string> tools;
    std::map<std::string, std::string> nodes; // id -> label
    const std::string dataPath = "../../Data";

    // Load tools
    XmlReader xr("archetypalTools.xml");
    for (auto elem : xr.getChildren("tool")){
        std::string id, archetype;
        if (!xr.findAttr(elem, "id", id)) continue;
        if (!xr.findAttr(elem, "archetype", archetype)) continue;
        tools[id] = archetype;
    }

    // Load items
    if (!xr.newFile(dataPath + "/items.xml"))
        std::cerr << "Failed to load items.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("item")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            std::string label = "item_" + id;

            std::string s;
            if (xr.findAttr(elem, "name", s))
                nodes.insert(std::make_pair(label, s));

            if (xr.findAttr(elem, "constructs", s)){
                    map.insert(std::make_pair(label, "object_" + s));
            }
        }
    }

    // Load objects
    if (!xr.newFile(dataPath + "/objectTypes.xml"))
        std::cerr << "Failed to load objectTypes.xml" << std::endl;
    else{
        for (auto elem : xr.getChildren("objectType")) {
            std::string id;
            if (!xr.findAttr(elem, "id", id))
                continue;
            std::string label = "object_" + id;

            std::string s;
            if (xr.findAttr(elem, "name", s))
                nodes.insert(std::make_pair(label, s));

            if (xr.findAttr(elem, "gatherReq", s)){
                auto it = tools.find(s);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << s << std::endl;
                    map.insert(std::make_pair(s, label));
                } else
                    map.insert(std::make_pair(it->second, label));
            }
            if (xr.findAttr(elem, "deconstructs", s)){
                map.insert(std::make_pair(label, "item_" + s));
            }
            for (auto yield : xr.getChildren("yield", elem)) {
                if (!xr.findAttr(yield, "id", s))
                    continue;
                map.insert(std::make_pair(label, "item_" + s));
            }
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
                map.insert(std::make_pair(label, "item_" + s));
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
            product = "item_" + product;

            std::string s;
            for (auto material : xr.getChildren("material", elem))
                if (xr.findAttr(material, "id", s))
                    map.insert(std::make_pair("item_" + s, product));

            for (auto material : xr.getChildren("tool", elem)){
                if (!xr.findAttr(material, "class", s))
                    continue;
                auto it = tools.find(s);
                if (it == tools.end()){
                    std::cerr << "Tool class is missing archetype: " << s << std::endl;
                    map.insert(std::make_pair(s, product));
                } else
                    map.insert(std::make_pair(it->second, product));
            }
        }
    }

    // Remove shortcuts and loops
    for (auto edgeIt = map.begin(); edgeIt != map.end(); ){
        /*
        This is one edge.  We want to figure out if there's a longer path here
        (i.e., this path doesn't add any new information about progress requirements).
        */
        bool shouldDelete = false;
        std::queue<const std::string> queue;
        std::string &target = edgeIt->second;

        // Populate queue initially with all direct children
        auto vals = map.equal_range(edgeIt->first);
        for (auto it = vals.first; it != vals.second; ++it)
            if (it->second != target)
                queue.push(it->second);

        // New nodes to check will be added here.  Effectively it will be a breadth-first search.
        while (!queue.empty() && !shouldDelete){
            std::string nextParent = queue.front();
            queue.pop();
            auto vals = map.equal_range(nextParent);
            for (auto it = vals.first; it != vals.second; ++it){
                if (it->second == target){
                    // Mark the edge for removal
                    shouldDelete = true;
                    break;
                }
            }
        }
        auto nextIt = edgeIt; ++nextIt;
        if (shouldDelete)
            map.erase(edgeIt);
        edgeIt = nextIt;
    }


    // Publiah
    std::ofstream f("tree.gv");
    f << "digraph {";
    for (auto &node : nodes)
        f << node.first << " [label=\"" << node.second << "\"]" << std::endl;

    for (auto &edge : map)
        f << edge.first << " -> " << edge.second << std::endl;
    f << "}";
}
