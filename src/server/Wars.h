#ifndef WARS_H
#define WARS_H

#include <map>
#include <string>
#include <vector>

struct Belligerent {
  enum Type { CITY, PLAYER };

  Type type;
  std::string name;

  Belligerent(const std::string &name = "", Type type = PLAYER)
      : type(type), name(name) {}
  Belligerent(const char *name, Type type = PLAYER) : type(type), name(name) {}

  bool operator==(const Belligerent &rhs) const;
  bool operator!=(const Belligerent &rhs) const;
  bool operator<(const Belligerent &rhs) const;

  void alertToWarWith(const Belligerent rhs) const;
};

class War {
  enum PeaceState {
    NO_PEACE_PROPOSED = 0,
    PEACE_PROPOSED_BY_B1 = 1,
    PEACE_PROPOSED_BY_B2 = 2
  };

  War(const Belligerent &b1, const Belligerent &b2);
  Belligerent b1, b2;  // b1 < b2
  PeaceState peaceState = NO_PEACE_PROPOSED;

  friend class Wars;

 public:
  bool operator<(const War &rhs) const;

  bool wasPeaceProposedBy(const Belligerent b) const;
};

class Wars {
 public:
  typedef std::set<War> container_t;

  void declare(const Belligerent &a, const Belligerent &b);
  bool isAtWar(Belligerent a, Belligerent b)
      const;  // Active wars only (taking into account cities)
  bool isAtWarExact(Belligerent a, Belligerent b) const;
  void sendWarsToUser(const User &user, const Server &server) const;
  void Wars::sendWarsInvolvingBelligerentToUser(const User &user,
                                                const Belligerent &belligerent,
                                                const Server &server) const;
  void sueForPeace(const Belligerent &proposer, const Belligerent &enemy);
  bool acceptPeaceOffer(const Belligerent &accepter,
                        const Belligerent &proposer);
  // Return value: whether there was a peace offer that was successfully revoked
  bool cancelPeaceOffer(const Belligerent &proposer, const Belligerent &enemy);

  void writeToXMLFile(const std::string &filename) const;
  void readFromXMLFile(const std::string &filename);

  static void changePlayerBelligerentToHisCity(Belligerent &belligerent);

 private:
  container_t container;
};

#endif
