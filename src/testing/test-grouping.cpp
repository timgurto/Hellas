#include "testing.h"

TEST_CASE("Shared XP") {
  GIVEN("Two users are in a group") {
    WHEN("One kills an NPC") {
      THEN("Both get XP") {}
    }
  }
}

// Shared XP
// Shared XP only if nearby
// Round-robin loot
// If loot is left, then anyone [in group] can pick it up
// /roll
// Show group members on map
// Leader can kick/invite
// Group chat
// City chat
// Invite if target is not in a group
// If in a group, only leader can invite
// Disappears when down to one member
