#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

typedef struct
{
    float position[3];
    float texcoord[2];
    float normal[3];
    float color[4];
} Vertex;

static const Vertex cube_vertex_list[] =
{
    // Front face
    // First triangle
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}},
    {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}},
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}},
    // Second triangle
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}},
    {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}},

    // Back face
    // First triangle
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}},
    {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}},
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}},
    // Second triangle
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}},
    {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}},
};

int main()
{
    C2D_SpriteSheet atlasSheet = C2D_SpriteSheetLoad("./atlas.t3x");
    C2D_Image img = C2D_SpriteSheetGetImage(atlasSheet, 0);
    C3D_Tex texture = *img.tex;
    return 0;
}