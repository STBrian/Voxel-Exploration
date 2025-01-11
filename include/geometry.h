#pragma once

#include <stdint.h>

typedef struct
{
    float position[3];
    float texcoord[2];
    float normal[3];
} Vertex;

typedef struct
{
    uint8_t position[3];
    float texcoord[2];
    uint8_t normal;
} CompressedVertex;

typedef struct 
{
    Vertex vertex[4];
} CubeFace;

typedef struct
{
    CubeFace front;
    CubeFace back;
    CubeFace right;
    CubeFace left;
    CubeFace top;
    CubeFace bottom;
} CubeGeo;

extern const CubeGeo cube;

extern const uint16_t vindexes[6];