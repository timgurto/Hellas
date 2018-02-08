#include <cassert>

#include "Client.h"
#include "Tag.h"
#include "../XmlReader.h"

const std::string & TagNames::operator[](const std::string & id) const {
    auto it = container_.find(id);
    if (it == container_.end())
        return id;
    return it->second;
}

void TagNames::readFromXMLFile(const std::string &filename) {
    XmlReader xr(filename);
    if (!xr) {
        Client::instance().showErrorMessage("Failed to load data from "s + filename, Color::FAILURE);
        return;
    }
    for (auto elem : xr.getChildren("tag")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) ||
            !xr.findAttr(elem, "name", name)) {
            Client::instance().showErrorMessage("Skipping tag with insufficient info."s, Color::FAILURE);
            continue;
        }
        container_[id] = name;
    }
}
