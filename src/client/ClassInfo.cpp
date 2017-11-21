#include "ClassInfo.h"
#include "Client.h"

ClassInfo::ClassInfo(const Name &name) : _name(name), _trees{} {
    _image = { "Images/Humans/" + name + ".png", Color::MAGENTA };
}

ClientTalent::ClientTalent(const Name & talentName, const Tree & treeName, const ClientSpell * spell) :
name(talentName),
tree(treeName),
spell(spell),
icon(spell->icon()){
    auto &client = Client::instance();
    learnMessage = Client::compileMessage(CL_TAKE_TALENT, talentName);
}
