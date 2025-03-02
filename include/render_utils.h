#pragma once

#include <citro3d.h>

#include <stdint.h>
#include <stdbool.h>

#include "geometry.h"

bool loadTexture(C3D_Tex *outTex, const char *fp);

uint8_t encodeNormalComponent(float value);

uint8_t compressNormals(float x, float y, float z);

void addCubeFaceToMesh(CompressedVertex *vbo, uint16_t *ibo, uint32_t vbo_idx, uint32_t ibo_idx, uint16_t face_idx, CubeFace face, int x, int y, int z);