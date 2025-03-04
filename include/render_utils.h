#pragma once

#include <citro3d.h>

#include <stdint.h>
#include <stdbool.h>

#include "geometry.h"
#include "render3d.h"

bool loadTexture(C3D_Tex *outTex, const char *fp);

uint8_t encodeNormalComponent(float value);

uint8_t compressNormals(float x, float y, float z);

void addCubeFaceToCubicMesh(CubicInstance *instance, CubeFace face, int x, int y, int z);