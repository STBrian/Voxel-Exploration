#pragma once

#include <stdint.h>

typedef struct vertex_s
{
    float position[3];
    float texcoord[2];
    float normal[3];
} Vertex;

typedef struct compressed_vertex_s
{
    int16_t position; // Used -> 0|00000,00000,00000| -> |z,y,x|
    uint8_t texcoord[2]; // Used -> all
    uint8_t normal; // Used -> 00|00,00,00| -> |z,y,x|
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