#include "geometry.h"

const CubeGeo cube = 
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

const uint16_t vindexes[6] = {
    0, 1, 2,
    2, 3, 0
};