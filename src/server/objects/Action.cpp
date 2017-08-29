#include "Action.h"
#include "../Server.h"

Action::FunctionMap Action::functionMap = {
    {"createCityWithRandomName", Server::createCityWithRandomName }
};

void Server::createCityWithRandomName(const Object & obj, User & performer) {
    auto &server = Server::instance();

    auto cityName = std::string{};
    for (auto i = 0; i != 10; ++i)
        cityName.push_back('a' + rand() % 26);
    server._cities.createCity(cityName);
    server._cities.addPlayerToCity(performer, cityName);
}
