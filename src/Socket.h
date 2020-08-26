#ifndef SOCKET_H
#define SOCKET_H

#include <windows.h>

#include <iostream>
#include <map>
#include <string>

#include "Log.h"
#include "types.h"

struct Message;

// Wrapper class for Winsock's SOCKET.
class Socket {
 public:
  static int sockAddrSize;
  static Log *debug;

 private:
  static WSADATA _wsa;
  static bool _winsockInitialized;
  SOCKET _raw;
  std::string _ip;
  ms_t _lingerTime;
  static std::map<SOCKET, int>
      _refCounts;  // Reference counters for each raw SOCKET
  bool _isBound{false};

  static void initWinsock();
  void addRef();                              // Increment reference counter
  static int closeRawAfterDelay(void *data);  // Thread function
  void close();  // Decrement reference counter, and close socket if no
                 // references remain

 public:
  Socket(const Socket &rhs);
  Socket();
  Socket(SOCKET &rawSocket, const std::string &ip);
  static Socket Empty();
  ~Socket();
  const Socket &operator=(const Socket &rhs);

  bool operator==(const Socket &rhs) const { return _raw == rhs._raw; }
  bool operator!=(const Socket &rhs) const { return _raw != rhs._raw; }
  bool operator<(const Socket &rhs) const { return _raw < rhs._raw; }

  void bind(sockaddr_in &socketAddr);
  void listen();
  bool isBound() const { return _isBound; }

  // Whether this socket is safe to use
  bool valid() const { return _raw != INVALID_SOCKET; }

  SOCKET getRaw() const { return _raw; }
  const std::string &ip() const { return _ip; }
  void delayClosing(ms_t lingerTime);  // Delay closing of socket

  // No destination socket implies client->server message
  void sendMessage(const Message &msg) const;
  void sendMessage(const Message &msg, const Socket &destSocket) const;
};

#endif
