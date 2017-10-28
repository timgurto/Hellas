#include <cassert>

#include "Client.h"
#include "ClientSpell.h"

ClientSpell::ClientSpell(const std::string &id) :
    _castMessage(Client::compileMessage(CL_CAST, id)),
    _icon("Images/Spells/"s + id + ".png"s){
    
    assert(_icon);
}
