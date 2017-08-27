#pragma once

#include <map>
#include <string>

class TagNames {
    using Container = std::map <std::string, std::string>;
    Container container_;

public:
    const std::string &operator[](const std::string &id) const;

    void readFromXMLFile(const std::string &filename);
};
