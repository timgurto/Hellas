#ifndef RECIPE_H
#define RECIPE_H

#include <set>
#include <string>

#include "JsonWriter.h"

struct Recipe{
    std::set<std::string> ingredients;
    std::set<std::string> tools;
    std::string time;
    std::string quantity;

    void writeToJSON(JsonWriter &jw){
        if (!ingredients.empty())
            jw.addArrayAttribute("craftingMats", ingredients, true);
        if (!tools.empty())
            jw.addArrayAttribute("craftingTools", tools);
        if (!time.empty())
            jw.addAttribute("craftingTime", time);
        if (!quantity.empty())
            jw.addAttribute("craftingQty", quantity);
    }
};

#endif
