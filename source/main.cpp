#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <fstream>
#include <vector>
#include <cstdint>

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
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    // Second triangle
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

    // Back face
    // First triangle
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    // Second triangle
    {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

    // Right face
	// First triangle
	{ {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, +0.5f, -0.5f}, {1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	// Second triangle
	{ {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Left face
	// First triangle
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	// Second triangle
	{ {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Top face
	// First triangle
	{ {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, +0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	// Second triangle
	{ {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Bottom face
	// First triangle
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	// Second triangle
	{ {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
};

#define cube_vertex_count (sizeof(cube_vertex_list)/sizeof(Vertex))

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
static C3D_Mtx projection;
static C3D_Mtx material =
{
	{
	{ { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
	{ { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
	{ { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
	{ { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
	}
};

void initScene();

int main()
{
    //C2D_SpriteSheet atlasSheet = C2D_SpriteSheetLoad("./atlas.t3x");
    //C2D_Image img = C2D_SpriteSheetGetImage(atlasSheet, 0);
    //C3D_Tex texture = *img.tex;
    romfsInit();


    return 0;
}

void initScene()
{
    std::ifstream file("romfs:/shaders/full_block.shbin", std::ios::binary | std::ios::ate);
    if (file) {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        uint8_t *buffer = new uint8_t[size];
        if (file.read(reinterpret_cast<char *>(buffer), size)) {
            vshader_dvlb = DVLB_ParseFile(reinterpret_cast<uint32_t *>(buffer), size / 4);
            shaderProgramInit(&program);
            shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
            C3D_BindProgram(&program);

            // Get the location of the uniforms
            uLoc_projection   = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
            uLoc_modelView    = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
            uLoc_lightVec     = shaderInstanceGetUniformLocation(program.vertexShader, "lightVec");
            uLoc_lightHalfVec = shaderInstanceGetUniformLocation(program.vertexShader, "lightHalfVec");
            uLoc_lightClr     = shaderInstanceGetUniformLocation(program.vertexShader, "lightClr");
            uLoc_material     = shaderInstanceGetUniformLocation(program.vertexShader, "material");

            // Configure attributes for use with the vertex shader
            C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
            AttrInfo_Init(attrInfo);
            AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
            AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
            AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal
            AttrInfo_AddLoader(attrInfo, 3, GPU_FLOAT, 4); // v2=color

            // Compute the projection matrix
	        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        }
    }
}