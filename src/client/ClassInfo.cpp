#include "ClassInfo.h"
#include "Client.h"

ClassInfo::ClassInfo(const Name &name) : _name(name), _trees{} {
    _image = { "Images/Humans/" + name + ".png", Color::MAGENTA };
}

void ClassInfo::addSpell(const ClientTalent::Name & talentName, const ClientTalent::Tree & tree, const ClientSpell * spell) {
    auto t = ClientTalent{ talentName, tree, ClientTalent::SPELL };
    t.spell = spell;
    t.icon = &spell->icon();
    _talents.insert(t);
}

void ClassInfo::addStats(const ClientTalent::Name & talentName, const ClientTalent::Tree & tree, const StatsMod & stats) {
    auto t = ClientTalent{ talentName, tree, ClientTalent::STATS };
    t.stats = stats;
    _talents.insert(t);
}

ClientTalent::ClientTalent(const Name & talentName, const Tree & treeName, Type type) :
name(talentName),
tree(treeName),
type(type){
    auto &client = Client::instance();
    learnMessage = Client::compileMessage(CL_TAKE_TALENT, talentName);
}
