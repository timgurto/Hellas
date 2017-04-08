#ifndef NODE_H
#define NODE_H

#include <map>
#include <set>
#include <string>

#include "types.h"

enum NodeType{
    ITEM,
    OBJECT,
    NPC
};

class Node{
public:
    typedef std::string Name;
    
private:
    typedef std::string DisplayName;

    ID id;
    NodeType type;
    Name name;
    DisplayName displayName;
    ID image;
    mutable bool imageExists;

    static const std::string &typePrefix(NodeType type);

    Node(Name name);


    void generateImage() const;

public:
    Node(NodeType type, const ID &id, const DisplayName &displayName);
    static Node dummy(Name name){
        return Node(name);
    }

    bool operator==(const Name &rhs) const;
    bool operator<(const Node &rhs) const;

    void outputAsGraphviz(std::ostream &output) const;

    void customImage(const ID &imageID){ image = imageID; }

    friend class Nodes;
};

class Nodes{
    std::set<Node> set;
public:
    void add(const Node &node);
    void remove(const Node::Name &name);
    void outputAsGraphviz(std::ostream &output) const;
    void generateAllImages() const;
};

#endif
