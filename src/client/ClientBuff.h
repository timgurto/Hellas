#pragma once

#include "ClientSpell.h"
#include "Texture.h"
#include "../Stats.h"

class ClientBuffType {
public:
    using ID = std::string;
    using Name = std::string;

    ClientBuffType() {}
    ClientBuffType(const ID &id);
    void name(const Name &name) { _name = name; }
    const Name &name() const { return _name; }
    const Texture &icon() const { return _icon; }
    void effectName(const std::string &name) { _effectName = name; }
    const std::string &effectName() const { return _effectName; }
    void effectArgs(const ClientSpell::Args &args) { _effectArgs = args; }
    const ClientSpell::Args &effectArgs() const { return _effectArgs; }
    void stats(const StatsMod &s) { _stats = s; }
    void duration(int s) { _duration = s; }
    const int duration() const { return _duration; }
    void tickTime(ms_t ms) { _tickTime = ms; }
    const ms_t tickTime() const { return _tickTime; }

private:
    Texture _icon;
    std::string _name{};

    int _duration{ 0 };
    ms_t _tickTime{ 0 };
    std::string _effectName{};
    ClientSpell::Args _effectArgs{};
    StatsMod _stats{};
};

using ClientBuffTypes = std::map<ClientBuffType::ID, ClientBuffType>;
