#pragma once

#include <cstdint>

#include "geometry.hpp"

inline void addCubeFaceToMesh(Vertex *vbo, uint16_t *ibo, uint32_t vbo_idx, uint32_t ibo_idx, uint16_t face_idx, CubeFace face, int x, int y, int z)
{
    for (int i = 0; i < 4; i++)
    {
        vbo[vbo_idx + i] = face.vertex[i];
        vbo[vbo_idx + i].position[0] += x;
        vbo[vbo_idx + i].position[1] += y;
        vbo[vbo_idx + i].position[2] += z;
    }

    for (int i = 0; i < 6; i++)
        ibo[ibo_idx + i] = vindexes[i] + face_idx * 4;
}