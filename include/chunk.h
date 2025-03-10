#pragma once

#include <citro3d.h>

#include <stdbool.h>
#include <stdlib.h>

#include "geometry.h"
#include "render3d.h"

typedef struct g_chunk {
    uint8_t size;
    uint16_t height;
    int x;
    int z;
    bool render_ready;
    CubicInstance *fragments;
    uint8_t** layered_blocks_data;
    uint16_t* facesPerFragment;
    struct g_chunk *front_neighbor; // z+
    struct g_chunk *back_neighbor; // z-
    struct g_chunk *right_neighbor; // x+
    struct g_chunk *left_neighbor; // x-
} Chunk;

void ChunkInit(Chunk* chunk_s, uint8_t size, uint16_t height);

void ChunkInitDefault(Chunk* chunk_s);

void ChunkSetBlock(Chunk* chunk_s, uint8_t x, uint8_t y, uint8_t z, uint8_t blockID);

uint8_t ChunkGetBlock(Chunk* chunk_s, int x, int y, int z);

void ChunkGenerateTerrain(Chunk* chunk_s, int seed);

void ChunkGenerateRenderObject(Chunk* chunk_s);

void ChunkRender(Chunk* chunk_s, RCamera* actorCamera);

void ChunkFreeRenderObject(Chunk* chunk_s);

void ChunkDestroy(Chunk* chunk_s);