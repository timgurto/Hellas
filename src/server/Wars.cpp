#include "Server.h"
#include "Wars.h"
#include "../XmlReader.h"
#include "../XmlWriter.h"


bool Wars::Belligerent::operator<(const Belligerent &rhs) const{
    if (name != rhs.name)
        return name < rhs.name;
    return type < rhs.type;
}

bool Wars::Belligerent::operator==(const Belligerent &rhs) const{
    return name == rhs.name && type == rhs.type;
}

void Wars::Belligerent::alertToWarWith(const Belligerent rhs) const{
    const Server &server = Server::instance();

    if (type == PLAYER)
        server.alertUserToWar(name, rhs);
    else{
        const auto members = server.cities().membersOf(name);
        for (const auto &member : members)
            server.alertUserToWar(member, rhs);
    }
}

void Wars::declare(const Belligerent &a, const Belligerent &b){
    if (!isAtWar(a, b))
        container.insert(std::make_pair(a, b));
    if (!isAtWar(b, a))
        container.insert(std::make_pair(b, a));

    a.alertToWarWith(b);
    b.alertToWarWith(a);
}

bool Wars::isAtWar(Belligerent a, Belligerent b) const{
    changePlayerBelligerentToHisCity(a);
    changePlayerBelligerentToHisCity(b);

    auto matches = container.equal_range(a);
    for (auto it = matches.first; it != matches.second; ++it){
        if (it->second == b)
            return true;
    }
    return false;
}

void Wars::changePlayerBelligerentToHisCity(Belligerent &belligerent){
    if (belligerent.type == Belligerent::CITY)
        return;
    
    const Server &server = Server::instance();
    const auto &playerCity = server.cities().getPlayerCity(belligerent.name);
    if (playerCity.empty())
        return;

    belligerent.type = Belligerent::CITY;
    belligerent.name = playerCity;
}

std::pair<Wars::container_t::const_iterator, Wars::container_t::const_iterator>
        Wars::getAllWarsInvolving(Belligerent a) const{
    changePlayerBelligerentToHisCity(a);
    return container.equal_range(a);
}

void Wars::writeToXMLFile(const std::string &filename) const{
    XmlWriter xw(filename);
    for (const Wars::Belligerents &belligerents : container) {
        auto e = xw.addChild("war");
        xw.setAttr(e, "name1", belligerents.first.name);
        if (belligerents.first.type == Belligerent::CITY)
            xw.setAttr(e, "isCity1", 1);
        xw.setAttr(e, "name2", belligerents.second.name);
        if (belligerents.second.type == Belligerent::CITY)
            xw.setAttr(e, "isCity2", 1);
    }
    xw.publish();
}

void Wars::readFromXMLFile(const std::string &filename){
    XmlReader xr(filename);
    if (! xr){
        Server::debug()("Failed to load data from " + filename, Color::FAILURE);
        return;
    }
    for (auto elem : xr.getChildren("war")) {
        Wars::Belligerent b1, b2;
        if (!xr.findAttr(elem, "name1", b1.name) ||
            !xr.findAttr(elem, "name2", b2.name)) {
                Server::debug()("Skipping war with insufficient belligerents.", Color::RED);
                continue;
        }
        int n;
        if (xr.findAttr(elem, "isCity1", n) && n != 0)
            b1.type = Belligerent::CITY;
        if (xr.findAttr(elem, "isCity2", n) && n != 0)
            b2.type = Belligerent::CITY;
        declare(b1, b2);
    }
}
