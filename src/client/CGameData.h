#pragma once

#include <map>
#include <set>
#include <string>

#include "CRecipe.h"
#include "ClientBuff.h"
#include "ClientItem.h"
#include "ClientNPC.h"
#include "ClientSpell.h"
#include "ClientTerrain.h"
#include "ParticleProfile.h"
#include "Tag.h"

struct CGameData {
  std::map<std::string, ClientItem> items;
  std::map<char, ClientTerrain> terrain;
  std::set<CRecipe> recipes;
  std::map<std::string, CNPCTemplate> npcTemplates;
  TagNames tagNames;
  ClientSpells spells;
  ClientBuffTypes buffTypes;

  using ObjectTypes =
      std::set<const ClientObjectType *, ClientObjectType::ptrCompare>;
  ObjectTypes objectTypes;

  using ParticleProfiles =
      std::set<const ParticleProfile *, ParticleProfile::ptrCompare>;
  ParticleProfiles particleProfiles;

  using ProjectileTypes =
      std::set<const Projectile::Type *, Projectile::Type::ptrCompare>;
  ProjectileTypes projectileTypes;
};
