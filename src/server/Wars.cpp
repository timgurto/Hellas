#include "Wars.h"

#include "../XmlReader.h"
#include "../XmlWriter.h"
#include "Server.h"

bool Belligerent::operator<(const Belligerent &rhs) const {
  if (name != rhs.name) return name < rhs.name;
  return type < rhs.type;
}

bool Belligerent::operator==(const Belligerent &rhs) const {
  return name == rhs.name && type == rhs.type;
}

bool Belligerent::operator!=(const Belligerent &rhs) const {
  return !(*this == rhs);
}

void Belligerent::alertToWarWith(const Belligerent rhs) const {
  const Server &server = Server::instance();

  if (type == PLAYER)
    server.alertUserToWar(name, rhs, false);
  else {
    const auto members = server.cities().membersOf(name);
    for (const auto &member : members) server.alertUserToWar(member, rhs, true);
  }
}

War::War(const Belligerent &b1, const Belligerent &b2) {
  if (b1 < b2) {
    this->b1 = b1;
    this->b2 = b2;
  } else if (b2 < b1) {
    this->b1 = b2;
    this->b2 = b1;
  } else
    assert(false);
}

bool War::operator<(const War &rhs) const {
  if (b1 != rhs.b1) return b1 < rhs.b1;
  return b2 < rhs.b2;
}

bool War::wasPeaceProposedBy(const Belligerent b) const {
  if (peaceState == NO_PEACE_PROPOSED) return false;
  if (peaceState == PEACE_PROPOSED_BY_B1 && b == b1) return true;
  if (peaceState == PEACE_PROPOSED_BY_B2) return true;
  return false;
}

void Wars::declare(const Belligerent &a, const Belligerent &b) {
  if (isAtWar(a, b)) return;
  if (a == b) return;
  container.insert({a, b});

  Server::instance().giveWarDeclarationDebuffs(a);

  a.alertToWarWith(b);
  b.alertToWarWith(a);
}

bool Wars::isAtWar(Belligerent a, Belligerent b) const {
  changePlayerBelligerentToHisCity(a);
  changePlayerBelligerentToHisCity(b);

  return isAtWarExact(a, b);
}

bool Wars::isAtWarExact(Belligerent a, Belligerent b) const {
  if (a == b) return false;

  auto warExists = container.find({a, b}) != container.end();
  return warExists;
}

void Wars::changePlayerBelligerentToHisCity(Belligerent &belligerent) {
  if (belligerent.type == Belligerent::CITY) return;

  const Server &server = Server::instance();
  const auto &playerCity = server.cities().getPlayerCity(belligerent.name);
  if (playerCity.empty()) return;

  belligerent.type = Belligerent::CITY;
  belligerent.name = playerCity;
}

void Wars::sendWarsToUser(const User &user, const Server &server) const {
  sendWarsInvolvingBelligerentToUser(user, {user.name(), Belligerent::PLAYER},
                                     server);

  auto cityName = server.cities().getPlayerCity(user.name());
  if (cityName.empty()) return;
  sendWarsInvolvingBelligerentToUser(user, {cityName, Belligerent::CITY},
                                     server);
}

void Wars::sendWarsInvolvingBelligerentToUser(const User &user,
                                              const Belligerent &belligerent,
                                              const Server &server) const {
  for (const auto &war : container) {
    auto enemy = Belligerent{};
    if (war.b1 == belligerent)
      enemy = war.b2;
    else if (war.b2 == belligerent)
      enemy = war.b1;
    else
      continue;

    auto warMessage = MessageCode{}, youProposedMessage = MessageCode{},
         proposedToYouMessage = MessageCode{};
    if (belligerent.type == Belligerent::PLAYER) {
      if (enemy.type == Belligerent::PLAYER) {
        warMessage = SV_AT_WAR_WITH_PLAYER;
        youProposedMessage = SV_YOU_PROPOSED_PEACE_TO_PLAYER;
        proposedToYouMessage = SV_PEACE_WAS_PROPOSED_TO_YOU_BY_PLAYER;
      } else {
        warMessage = SV_AT_WAR_WITH_CITY;
        youProposedMessage = SV_YOU_PROPOSED_PEACE_TO_CITY;
        proposedToYouMessage = SV_PEACE_WAS_PROPOSED_TO_YOUR_CITY_BY_PLAYER;
      }
    } else {
      if (enemy.type == Belligerent::PLAYER) {
        warMessage = SV_YOUR_CITY_AT_WAR_WITH_PLAYER;
        youProposedMessage = SV_YOU_PROPOSED_PEACE_TO_PLAYER;
        proposedToYouMessage = SV_PEACE_WAS_PROPOSED_TO_YOU_BY_CITY;
      } else {
        warMessage = SV_YOUR_CITY_AT_WAR_WITH_CITY;
        youProposedMessage = SV_YOUR_CITY_PROPOSED_PEACE_TO_CITY;
        proposedToYouMessage = SV_PEACE_WAS_PROPOSED_TO_YOUR_CITY_BY_CITY;
      }
    }

    user.sendMessage({warMessage, enemy.name});

    if (war.peaceState == War::NO_PEACE_PROPOSED) return;
    if (war.wasPeaceProposedBy(belligerent))
      user.sendMessage({youProposedMessage, enemy.name});
    else
      user.sendMessage({proposedToYouMessage, enemy.name});
  }
}

void Wars::sueForPeace(const Belligerent &proposer, const Belligerent &enemy) {
  auto it = container.find({proposer, enemy});
  if (it == container.end()) assert(false);
  auto &war = const_cast<War &>(*it);

  war.peaceState = war.b1 == proposer ? War::PEACE_PROPOSED_BY_B1
                                      : War::PEACE_PROPOSED_BY_B2;
}

bool Wars::acceptPeaceOffer(const Belligerent &accepter,
                            const Belligerent &proposer) {
  auto it = container.find({proposer, accepter});
  if (it == container.end()) assert(false);
  auto &war = const_cast<War &>(*it);

  // Make sure peace was proposed by proposer; Alice can't accept her own offer.
  if (war.b1 == proposer && war.peaceState != War::PEACE_PROPOSED_BY_B1)
    return false;
  if (war.b2 == proposer && war.peaceState != War::PEACE_PROPOSED_BY_B2)
    return false;

  container.erase(it);
  return true;
}

bool Wars::cancelPeaceOffer(const Belligerent &proposer,
                            const Belligerent &enemy) {
  auto it = container.find({proposer, enemy});
  if (it == container.end()) assert(false);
  auto &war = const_cast<War &>(*it);

  // Make sure peace was proposed by proposer; Alice can't revoke Bob's offer.
  if (war.b1 == proposer && war.peaceState != War::PEACE_PROPOSED_BY_B1)
    return false;
  if (war.b2 == proposer && war.peaceState != War::PEACE_PROPOSED_BY_B2)
    return false;

  war.peaceState = War::NO_PEACE_PROPOSED;
  return true;
}

void Wars::writeToXMLFile(const std::string &filename) const {
  XmlWriter xw(filename);
  for (const auto &war : container) {
    auto e = xw.addChild("war");

    xw.setAttr(e, "name1", war.b1.name);
    if (war.b1.type == Belligerent::CITY) xw.setAttr(e, "isCity1", 1);
    xw.setAttr(e, "name2", war.b2.name);
    if (war.b2.type == Belligerent::CITY) xw.setAttr(e, "isCity2", 1);

    if (war.peaceState != War::NO_PEACE_PROPOSED) {
      xw.setAttr(e, "peaceProposedBy", war.peaceState);
    }
  }
  xw.publish();
}

void Wars::readFromXMLFile(const std::string &filename) {
  auto xr = XmlReader::FromFile(filename);
  if (!xr) {
    Server::debug()("Failed to load data from " + filename, Color::CHAT_ERROR);
    return;
  }
  for (auto elem : xr.getChildren("war")) {
    Belligerent b1, b2;
    if (!xr.findAttr(elem, "name1", b1.name) ||
        !xr.findAttr(elem, "name2", b2.name)) {
      Server::debug()("Skipping war with insufficient belligerents.",
                      Color::CHAT_ERROR);
      continue;
    }
    int n;
    if (xr.findAttr(elem, "isCity1", n) && n != 0) b1.type = Belligerent::CITY;
    if (xr.findAttr(elem, "isCity2", n) && n != 0) b2.type = Belligerent::CITY;

    declare(b1, b2);

    auto peaceState = War::NO_PEACE_PROPOSED;
    if (xr.findAttr(elem, "peaceProposedBy", n)) {
      switch (n) {
        case 1:
          sueForPeace(b1, b2);
          break;
        case 2:
          sueForPeace(b2, b1);
          break;
        default:
          assert(false);
      }
    }
  }
}
