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
      {"MiEn", "the Giant", "might", "endurance"},
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
  const auto suffixSets = std::vector<SuffixSet>{
      {"2p1-d", 2, 2, 1, 17},    {"6p", 1, 1, 1, 10},
      {"7p", 1, 1, 1, 12},       {"8p", 2, 1, 1, 13},
      {"9p", 2, 2, 1, 15},       {"10p", 2, 2, 1, 17},
      {"11p", 2, 2, 1, 18},      {"12p", 2, 2, 1, 20},
      {"13p", 3, 2, 2, 22},      {"14p", 3, 2, 2, 23},
      {"15p", 3, 2, 2, 25},      {"16p", 3, 3, 2, 27},
      {"17p", 3, 3, 2, 28},      {"18p", 4, 3, 2, 30},
      {"19p", 4, 3, 2, 32},      {"20p", 4, 3, 3, 33},
      {"21p", 4, 3, 3, 35},      {"22p", 4, 3, 3, 37},
      {"23p", 5, 4, 3, 38},      {"24p", 5, 4, 3, 40},
      {"25p", 5, 4, 3, 42},      {"26p", 5, 4, 3, 43},
      {"27p", 5, 4, 4, 45},      {"28p", 6, 4, 4, 47},
      {"29p", 6, 4, 4, 48},      {"30p", 6, 5, 4, 50},
      {"31p", 6, 5, 4, 52},      {"32p", 6, 5, 4, 53},
      {"33p", 7, 5, 4, 55},      {"34p", 7, 5, 5, 57},
      {"35p", 7, 5, 5, 58},      {"36p", 7, 5, 5, 60},
      {"37p", 7, 6, 5, 62},      {"38p", 8, 6, 5, 63},
      {"39p", 8, 6, 5, 65},      {"40p", 8, 6, 5, 67},
      {"41p", 8, 6, 6, 68},      {"42p", 8, 6, 6, 70},
      {"43p", 9, 6, 6, 72},      {"44p", 9, 7, 6, 73},
      {"45p", 9, 7, 6, 75},      {"46p", 9, 7, 6, 77},
      {"47p", 9, 7, 6, 78},      {"48p", 10, 7, 7, 80},
      {"49p", 10, 7, 7, 82},     {"50p", 10, 7, 7, 83},
      {"51p", 10, 8, 7, 85},     {"52p", 10, 8, 7, 87},
      {"53p", 11, 8, 7, 88},     {"55p", 11, 8, 8, 92},
      {"56p", 11, 8, 8, 93},     {"57p", 11, 8, 8, 95},
      {"58p", 12, 9, 8, 97},     {"59p", 12, 9, 8, 98},
      {"60p", 12, 9, 8, 100},    {"61p", 12, 9, 8, 102},
      {"62p", 12, 9, 9, 103},    {"63p", 13, 9, 9, 105},
      {"64p", 13, 9, 9, 107},    {"65p", 13, 10, 9, 108},
      {"66p", 13, 10, 9, 110},   {"67p", 13, 10, 9, 112},
      {"68p", 14, 10, 9, 113},   {"69p", 14, 10, 10, 115},
      {"71p", 14, 10, 10, 118},  {"72p", 14, 11, 10, 120},
      {"73p", 15, 11, 10, 122},  {"74p", 15, 11, 10, 123},
      {"75p", 15, 11, 10, 125},  {"76p", 15, 11, 11, 127},
      {"77p", 15, 11, 11, 128},  {"78p", 16, 11, 11, 130},
      {"79p", 16, 12, 11, 132},  {"80p", 16, 12, 11, 133},
      {"82p", 16, 12, 11, 137},  {"83p", 17, 12, 12, 138},
      {"84p", 17, 12, 12, 140},  {"86p", 17, 13, 12, 143},
      {"87p", 17, 13, 12, 145},  {"89p", 18, 13, 12, 148},
      {"90p", 18, 13, 13, 150},  {"93p", 19, 14, 13, 155},
      {"95p", 19, 14, 13, 158},  {"96p", 19, 14, 13, 160},
      {"100p", 20, 15, 14, 167}, {"103p", 21, 15, 14, 172},
      {"104p", 21, 15, 15, 173}, {"107p", 21, 16, 15, 178},
      {"110p", 22, 16, 15, 183}, {"111p", 22, 16, 16, 185},
      {"114p", 23, 17, 16, 190}, {"115p", 23, 17, 16, 192},
      {"118p", 24, 17, 17, 197}, {"121p", 24, 18, 17, 202},
      {"122p", 24, 18, 17, 203}, {"123p", 25, 18, 17, 205},
      {"126p", 25, 18, 18, 210}, {"129p", 26, 19, 18, 215},
      {"130p", 26, 19, 18, 217}, {"134p", 27, 19, 19, 223},
      {"138p", 28, 20, 19, 230}, {"142p", 28, 21, 20, 237},
      {"143p", 29, 21, 20, 238}, {"148p", 30, 21, 21, 247},
      {"153p", 31, 22, 22, 255}, {"156p", 31, 23, 22, 260},
      {"157p", 31, 23, 22, 262}, {"165p", 33, 24, 23, 275},
      {"168p", 34, 24, 24, 280}, {"171p", 34, 25, 24, 285},
      {"176p", 35, 25, 25, 293}, {"186p", 37, 27, 26, 310},
      {"187p", 37, 27, 26, 312}, {"201p", 40, 29, 28, 335},
      {"217p", 43, 31, 31, 362}, {"233p", 47, 24, 33, 388},
      {"250p", 50, 36, 35, 417}, {"267p", 53, 38, 38, 445},
      {"285p", 57, 41, 40, 475}, {"303p", 61, 44, 43, 505}};

  for (auto set : suffixSets) {
    output << "<suffixSet id=\"" << set.id << "\">" << std::endl;
    generateSingleCompositeSuffixes(output, set.singleComposite);
    generateCompositePairSuffixes(output, set.doubleComposite1,
                                  set.doubleComposite2);
    generateResistanceSuffixes(output, set.resistance);
    output << "</suffixSet>" << std::endl;
  }
}
