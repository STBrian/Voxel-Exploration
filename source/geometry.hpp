#pragma once

#include <3ds.h>

#include <cstdint>

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

static const CubeGeo cube = 
{
    .front = {{
        {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}}, // V0
        {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}}, // V1
        {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}}, // V2
        {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f}}, // V3
    }},
    .back = {{
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, // V4
        {{-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // V5
        {{+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // V6
        {{+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, // V7
    }},
    .right = {{
        {{+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}}, // V8
        {{+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}}, // V9
        {{+0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}}, // V10
        {{+0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}}, // V11
    }},
    .left = {{
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, // V12
        {{-0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, // V13
        {{-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}}, // V14
        {{-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}}, // V15
    }},
    .top = {{
        {{-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f}}, // V16
        {{-0.5f, +0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}}, // V17
        {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f}}, // V18
        {{+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, +1.0f, 0.0f}}, // V19
    }},
    .bottom = {{
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // V20
        {{+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // V21
        {{+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}}, // V22
        {{-0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}}, // V23
    }}
};

static const u16 vindexes[] = {
    0, 1, 2,
    2, 3, 0
};