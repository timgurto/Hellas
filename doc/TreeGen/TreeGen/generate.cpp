#include <iostream>
#include <fstream>
#include <map>
#include <queue>
#include <XmlReader.h>

struct Path{
    std::string child;
    std::vector<std::string> parents;

    Path(const std::string &startNode):
        child(startNode)
    {
        parents.push_back(startNode);
    }
};

int main(){
    std::multimap<std::string, std::string> map; // map[a] = b: a -> b
    std::map<std::string, std::string> tools;
    std::map<std::string, std::string> nodes; // id -> label
    std::map<std::string, std::string> blacklist;
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
        if (!xr.findAttr(elem, "parent", parent)) continue;
        if (!xr.findAttr(elem, "child", child)) continue;
        blacklist[parent] = child;
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

    // Remove blacklisted items
    for (auto edge : blacklist){
        auto vals = map.equal_range(edge.first);
        for (auto it = vals.first; it != vals.second; ++it)
            if (it->second == edge.second){
                map.erase(it);
                break;
            }
    }


    // Remove shortcuts
    for (auto edgeIt = map.begin(); edgeIt != map.end(); ){
        /*
        This is one edge.  We want to figure out if there's a longer path here
        (i.e., this path doesn't add any new information about progress requirements).
        */
        bool shouldDelete = false;
        std::queue<std::string> queue;
        std::set<std::string> nodesFound;
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
                } else if (nodesFound.find(it->second) != nodesFound.end()){
                    queue.push(it->second);
                    nodesFound.insert(it->second);
                }
            }
        }
        auto nextIt = edgeIt; ++nextIt;
        if (shouldDelete)
            map.erase(edgeIt);
        edgeIt = nextIt;
    }


    //// Remove loops
    //// First, find set of nodes that start edges but don't finish them (i.e., the roots of the tree).
    //std::set<std::string> starts, ends;
    //for (auto &edge : map){
    //    starts.insert(edge.first);
    //    ends.insert(edge.second);
    //}
    //for (const std::string &endNode : ends){
    //    starts.erase(endNode);
    //}

    //// Next, do a BFS from each to find loops.  Remove those final edges.
    //std::set<std::pair<std::string, std::string> > trashCan;
    //for (const std::string &startNode : starts){
    //    //std::cout << "Root node: " << startNode << std::endl;
    //    std::queue<Path> queue;
    //    queue.push(Path(startNode));
    //    bool loopFound = false;
    //    while (!queue.empty() && !loopFound){
    //        Path nextPath = queue.front();
    //        queue.pop();
    //        auto vals = map.equal_range(nextPath.child);
    //        for (auto it = vals.first; it != vals.second; ++it){
    //            std::string child = it->second;
    //            // If this child is already a parent
    //            if (std::find(nextPath.parents.begin(), nextPath.parents.end(), child) != nextPath.parents.end()){
    //                std::cout << "Loop found; marked for removal:" << std::endl << "  ";
    //                for (const std::string &parent : nextPath.parents)
    //                    std::cout << parent << " -> ";
    //                std::cout << child << std::endl;
    //                // Mark the edge for removal
    //                trashCan.insert(*it);
    //            } else {
    //                Path p = nextPath;
    //                p.parents.push_back(child);
    //                p.child = child;
    //                queue.push(p);
    //            }
    //        }
    //    }
    //    for (auto pair : trashCan){
    //        auto vals = map.equal_range(pair.first);
    //        for (auto it = vals.first; it != vals.second; ++it){
    //            if (it->second == pair.second){
    //                map.erase(it);
    //                break;
    //            }
    //        }
    //    }
    //}



    // Publish
    std::ofstream f("tree.gv");
    f << "digraph {" << std::endl;
    f << "node [fontsize=10 shape=box];" << std::endl;

    // Nodes
    for (auto &node : nodes){
        std::string imagePath = "../../Images/";
        if (node.first.substr(0, 5) == "item_")
            imagePath += "items/" + node.first.substr(5);
        else if (node.first.substr(0, 4) == "npc_")
            imagePath += "NPCs/" + node.first.substr(4);
        else
            imagePath += "objects/" + node.first.substr(7);

        imagePath += ".png";
        std::string
            id = node.first,
            name = node.second,
            image = "<img src=\"" + imagePath + "\"/>",
            fullNode = node.first + " [label=<<table border='0' cellborder='0'><tr><td>" + image + "</td></tr><tr><td>" + name + "</td></tr></table>>]";
        f << fullNode << std::endl;
    }

    for (auto &edge : map)
        f << edge.first << " -> " << edge.second << std::endl;
    f << "}";
}
