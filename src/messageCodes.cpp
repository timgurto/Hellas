#include "messageCodes.h"

bool isMessageAllowedBeforeLogin(MessageCode message) {
  if (message == CL_PING) return true;
  if (message == CL_LOGIN_EXISTING) return true;
  if (message == CL_LOGIN_NEW) return true;
  return false;
}
