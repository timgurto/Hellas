#include "Exploration.h"

#include "Server.h"

Exploration::Exploration(size_t mapWidth, size_t mapHeight) {
  size_t chunksX = mapWidth / CHUNK_SIZE;
  size_t chunksY = mapHeight / CHUNK_SIZE;
  _map = {chunksX, std::vector<bool>(chunksY, false)};
}

void Exploration::sendSingleChunk(const Socket &socket, const Chunk &chunk) {
  Server::instance().sendMessage(socket, SV_CHUNK_EXPLORED,
                                 makeArgs(chunk.x, chunk.y));
}

Exploration::Chunk Exploration::getChunk(const MapPoint &location) {
  auto &map = Server::instance().map();
  auto row = map.getRow(location.y);
  auto col = map.getCol(location.x, row);
  return {col / CHUNK_SIZE, row / CHUNK_SIZE};
}

std::set<Exploration::Chunk> Exploration::explore(const Chunk &chunk) {
  auto newlyExploredChunks = std::set<Chunk>{};

  // 3x3 around central chunk
  auto minX = chunk.x > 0 ? chunk.x - 1 : 0,
       minY = chunk.y > 0 ? chunk.y - 1 : 0,
       maxX = min(chunk.x + 1, _map.size() - 1),
       maxY = min(chunk.y + 1, _map.front().size() - 1);
  for (auto x = minX; x <= maxX; ++x)
    for (auto y = minY; y <= maxY; ++y) {
      auto &isChunkExplored = _map[x][y];
      if (isChunkExplored) continue;

      isChunkExplored = true;
      newlyExploredChunks.insert(Chunk{x, y});
    }

  return newlyExploredChunks;
}

bool operator<(const Exploration::Chunk &lhs, const Exploration::Chunk &rhs) {
  if (lhs.x != rhs.x) return lhs.x < rhs.x;
  return lhs.y < rhs.y;
}
