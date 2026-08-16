#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <tinyxml.h>

#include <iostream>
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

bool ColoursEqual(const SDL_Color& a, const SDL_Color& b) {
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

      FOR_EACH_XML(colourIn, element, "colourIn") {
        const auto colourName = colourIn->Attribute("name");
        terrainTemplate.colourIn.push_back(colourName);
      }

      const char* name = element->Attribute("name");
      templates[name] = terrainTemplate;
    }

    // Terrains to generate
    FOR_EACH_XML(element, root, "terrain") {
      TerrainToGenerate terrain;

      terrain.name = element->Attribute("name");
      terrain.templateName = element->Attribute("template");

      FOR_EACH_XML(colourOut, element, "colourOut") {
        auto colourName = colourOut->Attribute("name");
        terrain.colourOut.push_back(colourName);
      }

      terrainsToGenerate.push_back(terrain);
    }
  }

  void ReplaceColours(SDL_Surface* surface,
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
          if (ColoursEqual({r, g, b, a}, mapping.first)) {
            *reinterpret_cast<Uint32*>(pixelAddress) =
                SDL_MapRGBA(surface->format, mapping.second.r, mapping.second.g,
                            mapping.second.b, a);
            break;
          }
        }
      }
    }

    SDL_UnlockSurface(surface);
  }

  void GenerateTerrain(const TerrainToGenerate& terrain) {
    const TerrainTemplate& terrainTemplate = templates.at(terrain.templateName);

    for (int variant = 0; variant < terrainTemplate.numVariants; ++variant) {
      std::string inputPath = "templates/" + terrain.templateName +
                              std::to_string(variant) + ".png";

      std::string outputPath =
          "generatedTiles/" + terrain.name + std::to_string(variant) + ".png";

      std::cout << inputPath << " -> " << outputPath << "\n";

      SDL_Surface* surface = IMG_Load(inputPath.c_str());

      ReplaceColours(surface, terrainTemplate, terrain);

      IMG_SavePNG(surface, outputPath.c_str());

      SDL_FreeSurface(surface);
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
    data.GenerateTerrain(terrain);
  }

  IMG_Quit();
  SDL_Quit();

  return 0;
}