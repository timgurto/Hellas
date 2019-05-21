#pragma once
#include <set>
#include <vector>

#include "../Point.h"

class Socket;

// Contained in User
class Exploration {
 public:
  static const auto CHUNK_SIZE = 20;  // In tiles

  using Chunk = Point<size_t>;

  Exploration(size_t mapWidth, size_t mapHeight);

  void sendSingleChunk(const Socket &socket, const Chunk &chunk);

  Chunk getChunk(const MapPoint &location);

  std::set<Chunk> explore(const Chunk &chunk);  // Returns newly explored chunks

 private:
  std::vector<std::vector<bool> > _map;
};

bool operator<(const Exploration::Chunk &lhs, const Exploration::Chunk &rhs);
