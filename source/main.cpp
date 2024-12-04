#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "geometry.hpp"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define CHUNK_SIZE 8

typedef struct {
    float position[3];
    float yaw;
    float pitch;
} Camera;

C2D_TextBuf g_textBuffer;
C2D_Text g_fpsText;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
static C3D_Mtx projection, projection_view;
static C3D_Mtx material =
{
	{
	{ { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
	{ { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
	{ { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
	{ { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
	}
};

static Vertex *vbo_data;
static u16 *ibo_data;
static Camera camera;

bool initScene();
void sceneRender();

int main()
{
    cfguInit(); // Allows system functions to work
    romfsInit();

    // Initialize graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Initialize the render target
	C3D_RenderTarget* target3D = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target3D, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    consoleInit(GFX_BOTTOM, NULL);

    g_textBuffer = C2D_TextBufNew(4096);

    //C2D_SpriteSheet atlasSheet = C2D_SpriteSheetLoad("./atlas.t3x");
    //C2D_Image img = C2D_SpriteSheetGetImage(atlasSheet, 0);
    //C3D_Tex texture = *img.tex;

    bool success = initScene();

    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
    float fps = 0.0f;
    std::string fpsString = "FPS: 0";

    int clearColor = 0x000000FF;

    // Main loop
	while (aptMainLoop() && success)
	{
        lastTime = std::chrono::high_resolution_clock::now();

		hidScanInput();

		// Respond to user input
        u32 kHeld = hidKeysHeld();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
        
        if (kHeld & KEY_UP)    camera.position[2] -= 0.1; // Mover hacia adelante
        if (kHeld & KEY_DOWN)  camera.position[2] += 0.1;  // Mover hacia atrás
        if (kHeld & KEY_A)  camera.position[1] += 0.1; // Rotar izquierda
        if (kHeld & KEY_B) camera.position[1] -= 0.1;  // Rotar derecha
        if (kHeld & KEY_LEFT)  camera.position[0] -= 0.1; // Rotar izquierda
        if (kHeld & KEY_RIGHT) camera.position[0] += 0.1;  // Rotar derecha
        if (kHeld & KEY_CSTICK_UP)  camera.pitch += 0.5; // Rotar izquierda
        if (kHeld & KEY_CSTICK_DOWN)  camera.pitch -= 0.5;
        if (kHeld & KEY_CSTICK_RIGHT)  camera.yaw += 0.5; // Rotar izquierda
        if (kHeld & KEY_CSTICK_LEFT)  camera.yaw -= 0.5; // Rotar izquierda

		// Need to configure every frame since Citro2D uses its own shader that broke all 3d rendering
        C3D_BindProgram(&program);

        C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
        AttrInfo_Init(attrInfo);
        AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
        AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
        AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal

        C3D_BufInfo *bufInfo = C3D_GetBufInfo();
        BufInfo_Init(bufInfo);
        BufInfo_Add(bufInfo, vbo_data, sizeof(Vertex), 3, 0x210);

        C3D_TexBind(0, NULL);

        C3D_TexEnv* env = C3D_GetTexEnv(0);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

        // Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target3D, C3D_CLEAR_ALL, clearColor, 0);
        C3D_FrameDrawOn(target3D);
        sceneRender();

        C2D_Prepare();
        C2D_SceneBegin(target3D);

        C2D_TextBufClear(g_textBuffer);
        C2D_TextParse(&g_fpsText, g_textBuffer, fpsString.c_str());
        C2D_TextOptimize(&g_fpsText);
        C2D_DrawText(&g_fpsText, C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));

		C3D_FrameEnd(0);

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;

        fps = 1.0f / elapsed.count();

        lastTime = currentTime;

        fpsString = "FPS: " + std::to_string((int)floor(fps));
	}

    // Free the VBO
	linearFree(vbo_data);
    linearFree(ibo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);

    C2D_TextBufDelete(g_textBuffer);

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

            // Initializa Camera
            camera = (Camera){
                {0.0f, 0.0f, 5.0f},
                0.0f,
                0.0f,
            };

            // Compute the view and projection matrix
	        Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

            // Create the Vertex Buffer Object and Index Buffer Object
            vbo_data = (Vertex *)linearAlloc(sizeof(cube_vertex_list)*CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE);
            ibo_data = (u16 *)linearAlloc(sizeof(vindexes)*6*CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE);
            for (int y = 0; y < CHUNK_SIZE; y++)
            {
                for (int x = 0; x < CHUNK_SIZE; x++)
                {
                    for (int z = 0; z < CHUNK_SIZE; z++)
                    {
                        int block_index = (y*CHUNK_SIZE*CHUNK_SIZE+x*CHUNK_SIZE+z);
                        int uindex = block_index*4*6;
                        for (int i = 0; i < 4*6; i++)
                        {
                            vbo_data[uindex + i] = cube_vertex_list[i];
                            vbo_data[uindex + i].position[0] += x;
                            vbo_data[uindex + i].position[1] += y;
                            vbo_data[uindex + i].position[2] += z;
                            vbo_data[uindex + i].normal[0] += x;
                            vbo_data[uindex + i].normal[1] += y;
                            vbo_data[uindex + i].normal[2] += z;
                        }

                        int vindex = block_index*6*6;
                        for (int i = 0; i < 6; i++)
                        {
                            for (int j = 0; j < 6; j++)
                            {
                                ibo_data[6 * i + j + vindex] = vindexes[j] + i * 4 + uindex;
                            }
                        }
                    }
                }
            }

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
    Mtx_RotateY(&modelView, C3D_AngleFromDegrees(camera.yaw), true);
    Mtx_RotateX(&modelView, C3D_AngleFromDegrees(camera.pitch), false);
    Mtx_Translate(&modelView, -camera.position[0], -camera.position[1], -camera.position[2], false);
    
    // Update uniforms 
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_material, &material);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec,     0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightHalfVec, 0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);

    C3D_DrawElements(GPU_TRIANGLES, 6*6*CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE, C3D_UNSIGNED_SHORT, ibo_data);
}