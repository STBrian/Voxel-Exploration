#pragma once

#include <citro3d.h>

#include <stdbool.h>
#include <stdlib.h>

#include "geometry.h"

typedef struct g_chunk {
    uint8_t size;
    uint16_t height;
    int x;
    int z;
    bool render_ready;
    uint8_t** layered_blocks_data;
    CompressedVertex** VBOs;
    uint16_t** IBOs;
    uint16_t* facesPerFragment;
} Chunk;

void ChunkInit(Chunk* chunk_s, uint8_t size, uint16_t height);

void ChunkInitDefault(Chunk* chunk_s);

void ChunkSetBlock(Chunk* chunk_s, uint8_t x, uint8_t y, uint8_t z, uint8_t blockID);

uint8_t ChunkGetBlock(Chunk* chunk_s, uint8_t x, uint8_t y, uint8_t z);

void ChunkGenerateTerrain(Chunk* chunk_s, int seed);

void ChunkGenerateRenderObject(Chunk* chunk_s);

void ChunkRender(Chunk* chunk_s);

void ChunkFreeRenderObject(Chunk* chunk_s);

void ChunkDestroy(Chunk* chunk_s);