#include "Action.h"

Action::FunctionMap Action::functionMap = {
    {"createCityWithRandomName", createCityWithRandomName }
};

void createCityWithRandomName(const Object & obj, User & performer) {
}
