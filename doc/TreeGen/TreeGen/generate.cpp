#include <iostream>
#include <fstream>
#include <map>
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


    // Load recipes


    // Publiah
    std::ofstream f("tree.gv");
    f << "digraph {";
    for (auto &node : nodes)
        f << node.first << " [label=\"" << node.second << "\"]" << std::endl;

    for (auto &edge : map)
        f << edge.first << " -> " << edge.second << std::endl;
    f << "}";
}
