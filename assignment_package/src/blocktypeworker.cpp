#include "blocktypeworker.h"
#include "iostream"

BlockTypeWorker::BlockTypeWorker(Terrain * terrain,
                                 int64_t hashCoord,
                                 std::vector<Chunk*> terrainsChunk,
                                 std::vector<Chunk *> *mp_chunksWithOnlyBlockData,
                                 QMutex* mutex)
    :mp_terrain(terrain), coord(hashCoord), terrainsChunk(terrainsChunk), mp_chunksWithOnlyBlockData(mp_chunksWithOnlyBlockData), mp_mutex(mutex)
{
}

void BlockTypeWorker::run() {
    // create 4 by 4 chunks and set its neighbors
    createChunksInTerrain();
}

void BlockTypeWorker::createChunksInTerrain() {
     int x = toCoords(coord).x;
     int z = toCoords(coord).y;

     // fill chunk with block
     for (int i = x; i < x + 64; i++) {
         for (int j = z; j < z + 64; j++) {
             try {
             mp_terrain->fillBlock(i, j);
             }
             catch(std::out_of_range &e) {
                 std::cout << "out of range in blocktypeworker";
             }
         }
     }
     River river = River(mp_terrain, x, z);
     Cave cave = Cave(mp_terrain, x, z);
     double random = ((double) rand() / (RAND_MAX));
     if (random < 0.15)
        river.draw();
     double random2 = ((double) rand() / (RAND_MAX));
     if (random2 < 0.15)
     {
        cave.carveOpening();
     }

    mp_mutex->lock();


    for (Chunk *c : terrainsChunk) {
         mp_terrain->chunksWithOnlyBlockData.push_back(c);
    }

    mp_mutex->unlock();
}
