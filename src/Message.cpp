#include "Message.h"

std::ostream& operator<<(std::ostream& lhs, const Message& rhs) {
  lhs << rhs.code << "(" << rhs.args << ")";
  return lhs;
}
