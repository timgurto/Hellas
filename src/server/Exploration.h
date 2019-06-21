#pragma once
#include <set>
#include <vector>

#include "../Point.h"

class Socket;
class XmlReader;
class XmlWriter;

// Contained in User
class Exploration {
 public:
  static const auto CHUNK_SIZE = 10;  // In tiles

  using Chunk = Point<size_t>;

  Exploration(size_t mapWidth, size_t mapHeight);

  void writeTo(XmlWriter &xw) const;
  void readFrom(XmlReader &xr);

  int numChunksExplored() const { return _numChunksExplored; }
  int numChunks() const { return _map.size() * _map.front().size(); }

  void sendWholeMap(const Socket &socket) const;
  void sendSingleChunk(const Socket &socket, const Chunk &chunk) const;

  Chunk getChunk(const MapPoint &location);

  std::set<Chunk> explore(const Chunk &chunk);  // Returns newly explored chunks
  void unexploreAll(const Socket &socket);

 private:
  std::vector<std::vector<bool> > _map;
  int _numChunksExplored{0};  // Used only for data logging.
};

bool operator<(const Exploration::Chunk &lhs, const Exploration::Chunk &rhs);
