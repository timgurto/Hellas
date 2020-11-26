#pragma once

#include "../types.h"

class AI {
 public:
  static const px_t AGGRO_RANGE{70};
  static const px_t PURSUIT_RANGE{245};  // Assumption: this is farther than any
                                         // ranged attack/spell can reach:
  static const px_t FOLLOW_DISTANCE{28};
  static const px_t MAX_FOLLOW_RANGE{210};
  static const ms_t FREQUENCY_TO_LOOK_FOR_TARGETS{250};

  AI(class NPC &owner);

  enum State { IDLE, CHASE, ATTACK, PET_FOLLOW_OWNER } state{IDLE};

 private:
  NPC &_owner;
};
