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

bool Exploration::explore(const Chunk &chunk) {
  auto &isChunkExplored = _map[chunk.x][chunk.y];
  auto wasAlreadyExplored = static_cast<bool>(isChunkExplored);
  isChunkExplored = true;
  return !wasAlreadyExplored;
}
