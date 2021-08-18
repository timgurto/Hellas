#include <fstream>
#include <string>
#include <vector>

void generateSingleCompositeSuffixes(std::ofstream& output, int statValue) {
  struct Composite {
    std::string suffixID;
    std::string name;
    std::string stat;
  };
  const auto composites = std::vector<Composite>{
      {"Mi", "Might", "might"},         {"In", "Intellect", "intellect"},
      {"Cu", "Cunning", "cunning"},     {"Sw", "Swiftness", "swiftness"},
      {"En", "Endurance", "endurance"}, {"Co", "Courage", "courage"}};
  for (auto composite : composites)
    output << "  <suffix id=\"" << composite.suffixID << "\" name=\""
           << composite.name << "\"><stats " << composite.stat << "=\""
           << statValue << "\"/></suffix>" << std::endl;
}

void generateCompositePairSuffixes(std::ofstream& output, int statValue1,
                                   int statValue2) {
  struct DoubleComposite {
    std::string suffixID;
    std::string name;
    std::string stat1;
    std::string stat2;
  };
  const auto doubleComposites = std::vector<DoubleComposite>{
      {"InMi", "the Dragon", "intellect", "might"},
      {"InCu", "the Siren", "intellect", "cunning"},
      {"InEn", "the Hydra", "intellect", "endurance"},
      {"InSw", "the Centaur", "intellect", "swiftness"},
      {"InCo", "the Chimera", "intellect", "courage"},
      {"MiCu", "the Sphinx", "might", "cunning"},
      {"MiEn", "the Giant", "might", "might"},
      {"MiSw", "the Gryphon", "might", "swiftness"},
      {"MiCo", "the Lion", "might", "courage"},
      {"CuEn", "the Nymph", "cunning", "endurance"},
      {"CuSw", "the Harpie", "cunning", "swiftness"},
      {"CuCo", "the Satyr", "cunning", "courage"},
      {"EnSw", "the Phoenix", "endurance", "swiftness"},
      {"EnCo", "the Tortoise", "endurance", "courage"},
      {"SwCo", "the Unicorn", "swiftness", "courage"}};
  for (auto doubleComposite : doubleComposites) {
    output << "  <suffix id=\"" << doubleComposite.suffixID << "\" ";
    output << "name=\"" << doubleComposite.name << "\">";

    output << "<stats ";
    output << doubleComposite.stat1 << "=\"" << statValue1 << "\" ";
    output << doubleComposite.stat2 << "=\"" << statValue2 << "\"/>";

    output << "</suffix>" << std::endl;
  }
}

void generateResistanceSuffixes(std::ofstream& output, int statValue) {
  struct Resistance {
    std::string suffixID;
    std::string name;
    std::string stat;
  };
  const auto resistances =
      std::vector<Resistance>{{"rA", "Air Resistance", "airResist"},
                              {"rE", "Earth Resistance", "earthResist"},
                              {"rF", "Fire Resistance", "fireResist"},
                              {"rW", "Water Resistance", "waterResist"}};
  for (auto resistance : resistances)
    output << "  <suffix id=\"" << resistance.suffixID << "\" name=\""
           << resistance.name << "\"><stats " << resistance.stat << "=\""
           << statValue << "\"/></suffix>" << std::endl;
}

int main() {
  auto output = std::ofstream{"output.xml"};

  struct SuffixSet {
    std::string id;
    int singleComposite;
    int doubleComposite1;
    int doubleComposite2;
    int resistance;
  };
  const auto suffixSets = std::vector<SuffixSet>{{"9p", 2, 2, 1, 15}};

  for (auto set : suffixSets) {
    output << "<suffixSet id=\"" << set.id << "\">" << std::endl;
    generateSingleCompositeSuffixes(output, set.singleComposite);
    generateCompositePairSuffixes(output, set.doubleComposite1,
                                  set.doubleComposite2);
    generateResistanceSuffixes(output, set.resistance);
    output << "</suffixSet>" << std::endl;
  }
}
