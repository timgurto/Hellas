#pragma once

class AI {
 public:
  AI(class NPC &owner);

  enum State { IDLE, CHASE, ATTACK, PET_FOLLOW_OWNER } state{IDLE};

 private:
  NPC &_owner;
};
