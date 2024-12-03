#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct
{
    float position[3];
    float texcoord[2];
    float normal[3];
    float color[4];
} Vertex;

static const Vertex cube_vertex_list[6][6] =
{
    {
        // Front face
        // First triangle
        {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Second triangle
        {{+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    },

    {
        // Back face
        // First triangle
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, +0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.5f, 1.0f, 1.0f}},
        // Second triangle
        {{+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.5f, 1.0f, 1.0f}},
        {{+0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.5f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    },

    {
        // Right face
        // First triangle
        { {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 1.0f, 1.0f}},
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 1.0f, 1.0f}},
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Second triangle
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        { {+0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {+1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        { {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 1.0f, 1.0f}},
    },

    {
        // Left face
        // First triangle
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        { {-0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        // Second triangle
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        { {-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    },

    {
        // Top face
        // First triangle
        { {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Second triangle
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        { {+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.5f, 1.0f, 1.0f}},
        { {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
    },

    {
        // Bottom face
        // First triangle
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 1.0f, 1.0f, 1.0f}},
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        // Second triangle
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        { {-0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
    },
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

static void *vbo_data;
static float angleX = 0.0, angleY = 0.0;

bool initScene();
void sceneRender();
void sceneExit();

int main()
{
    // Initialize graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Initialize the render target
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    //C2D_SpriteSheet atlasSheet = C2D_SpriteSheetLoad("./atlas.t3x");
    //C2D_Image img = C2D_SpriteSheetGetImage(atlasSheet, 0);
    //C3D_Tex texture = *img.tex;
    romfsInit();

    bool success = initScene();

    int clearColor = 0x68B0D8FF;

    // Main loop
	while (aptMainLoop() && success)
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, clearColor, 0);
			C3D_FrameDrawOn(target);
			sceneRender();
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize graphics
	C3D_Fini();
	gfxExit();
    romfsExit();

    return 0;
}

bool initScene()
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
            AttrInfo_AddLoader(attrInfo, 3, GPU_FLOAT, 4); // v3=color

            // Compute the projection matrix
	        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

            // Create the Vertex Buffer Object
            vbo_data = linearAlloc(sizeof(cube_vertex_list));
            memcpy(vbo_data, cube_vertex_list, sizeof(cube_vertex_list));

            // Configure buffers
            C3D_BufInfo *bufInfo = C3D_GetBufInfo();
            BufInfo_Init(bufInfo);
            BufInfo_Add(bufInfo, vbo_data, sizeof(Vertex), 4, 0x3210);

            C3D_TexBind(0, NULL);
            
            C3D_TexEnv *env = C3D_GetTexEnv(0);
            C3D_TexEnvInit(env);
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            delete[] buffer;
            return true;
        }
        else
        {
            delete[] buffer;
            return false;
        }
    }
    else
        return false;
}

void sceneRender()
{
    // Calculate the modelView matrix
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, 0.0, 0.0, -2.0 + 0.5*sinf(angleX), true);
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Rotate the cube each frame
	angleX += M_PI / 180;
	angleY += M_PI / 360;
    
    // Update uniforms 
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_material, &material);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec,     0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightHalfVec, 0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);

    C3D_DrawArrays(GPU_TRIANGLES, 0, cube_vertex_count);
}

void sceneExit()
{
	// Free the VBO
	linearFree(vbo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}