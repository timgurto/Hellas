#pragma once

template <typename T>
void MessageParser::parseSingleArg(T &arg, ArgPosition argPosition) {
  iss >> arg;
}

template <>
void MessageParser::parseSingleArg(std::string &arg, ArgPosition argPosition) {
  static const size_t BUFFER_SIZE = 1023;
  char buffer[BUFFER_SIZE + 1];
  const auto expectedDelimiter = argPosition == Last ? MSG_END : MSG_DELIM;
  iss.get(buffer, BUFFER_SIZE, expectedDelimiter);
  arg = {buffer};
}

template <typename T1>
bool MessageParser::parseArgs(T1 &arg1) {
  char del;

  parseSingleArg(arg1, Last);
  iss >> del;
  if (del != MSG_END) return false;

  return true;
}

template <typename T1, typename T2>
bool MessageParser::parseArgs(T1 &arg1, T2 &arg2) {
  char del;
  parseSingleArg(arg1, NotLast);
  iss >> del;
  if (del != MSG_DELIM) return false;

  parseSingleArg(arg2, Last);
  iss >> del;
  if (del != MSG_END) return false;

  return true;
}

template <typename T1, typename T2, typename T3>
bool MessageParser::parseArgs(T1 &arg1, T2 &arg2, T3 &arg3) {
  char del;
  parseSingleArg(arg1, NotLast);
  iss >> del;
  if (del != MSG_DELIM) return false;

  parseSingleArg(arg2, NotLast);
  iss >> del;
  if (del != MSG_DELIM) return false;

  parseSingleArg(arg3, Last);
  iss >> del;
  if (del != MSG_END) return false;

  return true;
}
