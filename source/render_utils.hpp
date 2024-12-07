#pragma once

#include <cstdint>

#include "geometry.hpp"

inline void addCubeFaceToMesh(CompressedVertex *vbo, uint16_t *ibo, uint32_t vbo_idx, uint32_t ibo_idx, uint16_t face_idx, CubeFace face, int x, int y, int z)
{
    for (int i = 0; i < 4; i++)
    {
        vbo[vbo_idx + i] = {
            .position = {(uint8_t)(face.vertex[i].position[0] + x + 0.5), (uint8_t)(face.vertex[i].position[1] + y + 0.5), (uint8_t)(face.vertex[i].position[2] + z + 0.5)},
            .texcoord = {face.vertex[i].texcoord[0], face.vertex[i].texcoord[0]},
            .normal = {encodeNormalComponent(face.vertex[i].normal[0]), encodeNormalComponent(face.vertex[i].normal[1]), encodeNormalComponent(face.vertex[i].normal[2])}
        };
    }

    for (int i = 0; i < 6; i++)
        ibo[ibo_idx + i] = vindexes[i] + face_idx * 4;
}