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
    typedef std::string Label;

    ID id;
    NodeType type;
    Name name;
    Label label;

    static const std::string &typePrefix(NodeType type);

    Node(Name name);

public:
    Node(NodeType type, const ID &id, const std::string &label);
    static Node dummy(Name name){
        return Node(name);
    }

    bool operator==(const Name &rhs) const;
    bool operator<(const Node &rhs) const;

    void outputAsGraphviz(std::ostream &output) const;
};

class Nodes{
    std::set<Node> set;
public:
    void add(const Node &node);
    void remove(const Node::Name &name);
    void outputAsGraphviz(std::ostream &output) const;
};

#endif
