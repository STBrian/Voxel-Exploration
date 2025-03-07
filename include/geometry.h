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
    uint8_t position[3]; // Used -> 000|00000| 000|00000|
    uint8_t texcoord[2]; // Used -> all
    uint8_t normal; // Used -> 00|000000|
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