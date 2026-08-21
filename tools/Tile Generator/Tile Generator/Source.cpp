#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <tinyxml.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define FOR_EACH_XML(var, parent, tag) \
  for (auto var = (parent)->FirstChildElement(tag); var; \
       var = (var)->NextSiblingElement(tag))

struct TerrainTemplate {
  int numVariants = 0;
  std::vector<std::string> colourIn;
};

struct TerrainToGenerate {
  std::string name;
  std::string templateName;
  std::vector<std::string> colourOut;
};

SDL_Color parseHexColour(const std::string& hex) {
  const unsigned int value = std::stoul(hex, nullptr, 16);
  SDL_Color colour;
  colour.r = static_cast<Uint8>((value >> 16) & 0xff);
  colour.g = static_cast<Uint8>((value >> 8) & 0xff);
  colour.b = static_cast<Uint8>(value & 0xff);
  colour.a = 0xff;

  return colour;
}

std::vector<std::string> splitByComma(const std::string& s) {
  std::vector<std::string> result;
  std::istringstream stream(s);
  std::string token;
  while (std::getline(stream, token, ','))
    result.push_back(token);
  return result;
}

bool coloursEqual(const SDL_Color& a, const SDL_Color& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b;
}

class Data {
 public:
  std::unordered_map<std::string, SDL_Color> namedColours;
  std::unordered_map<std::string, TerrainTemplate> templates;
  std::vector<TerrainToGenerate> terrainsToGenerate;

  void loadFromXML() {
    TiXmlDocument document("terrain.xml");

    document.LoadFile();

    TiXmlElement* root = document.FirstChildElement("root");

    // Colours
    FOR_EACH_XML(element, root, "colour") {
      const auto name = element->Attribute("name");
      const auto hex = element->Attribute("hex");

      namedColours[name] = parseHexColour(hex);
    }

    // Templates
    FOR_EACH_XML(element, root, "terrainTemplate") {
      TerrainTemplate terrainTemplate;

      const char* numVariants = element->Attribute("numVariants");
      terrainTemplate.numVariants = std::atoi(numVariants);
      terrainTemplate.colourIn = splitByComma(element->Attribute("colours"));

      const char* name = element->Attribute("name");
      templates[name] = terrainTemplate;
    }

    // Terrains to generate
    FOR_EACH_XML(element, root, "terrain") {
      TerrainToGenerate terrain;

      terrain.name = element->Attribute("name");
      terrain.templateName = element->Attribute("template");
      terrain.colourOut = splitByComma(element->Attribute("colours"));

      terrainsToGenerate.push_back(terrain);
    }
  }

  void replaceColours(SDL_Surface* surface,
                      const TerrainTemplate& terrainTemplate,
                      const TerrainToGenerate& terrain) {
    std::vector<std::pair<SDL_Color, SDL_Color>> mappings;

    for (size_t i = 0; i < terrainTemplate.colourIn.size(); ++i) {
      mappings.push_back({namedColours.at(terrainTemplate.colourIn[i]),
                          namedColours.at(terrain.colourOut[i])});
    }

    SDL_LockSurface(surface);

    for (int y = 0; y < surface->h; ++y) {
      for (int x = 0; x < surface->w; ++x) {
        Uint8* pixelAddress = static_cast<Uint8*>(surface->pixels) +
                              y * surface->pitch +
                              x * surface->format->BytesPerPixel;

        auto pixel = *reinterpret_cast<Uint32*>(pixelAddress);

        Uint8 r, g, b, a;
        SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

        for (const auto& mapping : mappings) {
          if (coloursEqual({r, g, b, a}, mapping.first)) {
            auto newPixel = SDL_MapRGBA(surface->format, mapping.second.r,
                                        mapping.second.g, mapping.second.b, a);
            *reinterpret_cast<Uint32*>(pixelAddress) = newPixel;
            break;
          }
        }
      }
    }

    SDL_UnlockSurface(surface);
  }

  void generateTerrain(const TerrainToGenerate& terrain) {
    const TerrainTemplate& terrainTemplate = templates.at(terrain.templateName);

    for (int variant = 0; variant < terrainTemplate.numVariants; ++variant) {
      const auto inputPath = "templates/" + terrain.templateName +
                             std::to_string(variant) + ".png";

      const auto variantSuffix =
          terrainTemplate.numVariants > 1 ? std::to_string(variant) : "";
      const auto outputPath =
          "../../Images/Terrain/" + terrain.name + variantSuffix + ".png";

      auto surface = IMG_Load(inputPath.c_str());
      replaceColours(surface, terrainTemplate, terrain);
      IMG_SavePNG(surface, outputPath.c_str());
      SDL_FreeSurface(surface);

      std::cout << inputPath << " -> " << outputPath << "\n";
    }
  }
};

int main(int argc, char* argv[]) {
  SDL_Init(0);
  const int imageFlags = IMG_INIT_PNG;
  IMG_Init(imageFlags);

  Data data;
  data.loadFromXML();

  for (const TerrainToGenerate& terrain : data.terrainsToGenerate) {
    data.generateTerrain(terrain);
  }

  IMG_Quit();
  SDL_Quit();

  return 0;
}