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
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "chunk.hpp"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

const int MAX_CONCURRENT_TASKS = 2;

typedef struct {
    C3D_FVec position;
    float yaw;
    float pitch;
    C3D_Mtx view;
} Camera;

C2D_TextBuf g_textBuffer;
C2D_Text g_fpsText;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
static C3D_Mtx material =
{
	{
	{ { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
	{ { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
	{ { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
	{ { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
	}
};

std::vector<Chunk> chunks;
static Camera camera;

bool initScene();
void prepare3DRendering();
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
    float sensitivility = 0.05f;
    float speed = 0.3f;

    // Main loop
	while (aptMainLoop() && success)
	{
        // To calculate frames
        lastTime = std::chrono::high_resolution_clock::now();

        circlePosition cStick;
        hidCstickRead(&cStick);
		hidScanInput();

		// Respond to user input
        u32 kHeld = hidKeysHeld();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

        // Camera movement
        camera.pitch -= cStick.dy * sensitivility;
        camera.yaw += cStick.dx * sensitivility;
        if (camera.pitch > 89.0f) camera.pitch = 89.0f;
        if (camera.pitch < -89.0f) camera.pitch = -89.0f;
        if (camera.yaw > 360.0f) camera.yaw -= 360.0f;
        if (camera.yaw < 0.0f) camera.yaw += 360.0f;

        float yawRad = C3D_AngleFromDegrees(camera.yaw);
        float pitchRad = C3D_AngleFromDegrees(camera.pitch);

        C3D_FVec forward = FVec3_New(
            cosf(pitchRad) * sinf(yawRad),
            sinf(pitchRad),
            cosf(pitchRad) * cosf(yawRad)
        );
        C3D_FVec right = FVec3_New(
            sinf(yawRad - M_PI_2),
            0.0f,
            cosf(yawRad - M_PI_2)
        );
        C3D_FVec up = FVec3_Cross(forward, right);
        C3D_FVec forwardXZ = FVec3_New(forward.x, 0.0f, forward.z);

        forward = FVec3_Normalize(forward);
        forwardXZ = FVec3_Normalize(forwardXZ);
        right = FVec3_Normalize(right);
        up = FVec3_Normalize(up);
        
        if (kHeld & KEY_UP)
            camera.position = FVec3_Add(camera.position, FVec3_Scale(forwardXZ, speed));
        if (kHeld & KEY_DOWN)
            camera.position = FVec3_Subtract(camera.position, FVec3_Scale(forwardXZ, speed));
        if (kHeld & KEY_RIGHT)
            camera.position = FVec3_Subtract(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_LEFT)
            camera.position = FVec3_Add(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_A)
            camera.position = FVec3_Subtract(camera.position, FVec3_New(0.0f, speed, 0.0f));
        if (kHeld & KEY_B)
            camera.position = FVec3_Add(camera.position, FVec3_New(0.0f, speed, 0.0f));
        
        Mtx_LookAt(&camera.view, camera.position, FVec3_Add(camera.position, forward), up, false);

		// Need to configure every frame since Citro2D uses its own shader that broke all 3d rendering
        prepare3DRendering();

        // Render the 3D scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target3D, C3D_CLEAR_ALL, clearColor, 0);
        C3D_FrameDrawOn(target3D);
        sceneRender();

        // Need to configure every frame since change Citro3d shader broke 2d rendering
        C2D_Prepare();

        // Render the 2D scene
        C2D_SceneBegin(target3D);

        C2D_TextBufClear(g_textBuffer);
        C2D_TextParse(&g_fpsText, g_textBuffer, fpsString.c_str());
        C2D_TextOptimize(&g_fpsText);
        C2D_DrawText(&g_fpsText, C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));

		C3D_FrameEnd(0);

        // Calculate frame time
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;

        fps = 1.0f / elapsed.count();

        lastTime = currentTime;

        fpsString = "FPS: " + std::to_string((int)floor(fps));
	}

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

void prepare3DRendering()
{
    C3D_BindProgram(&program);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal

    C3D_TexBind(0, NULL);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_BACK_CCW);
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

            // Initialize Camera
            camera = (Camera){
                .position = FVec3_New(0.0f, -60.0f, 0.0f),
                .yaw = 0.0f,
                .pitch = 0.0f
            };

            std::queue<std::future<void>> taskQueue;
            int currentTasks = 0;

            std::cout << "Generating chunk data..." << std::endl;
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    chunks.emplace_back(i, j);

            for (Chunk& chunk : chunks)
            {
                if (currentTasks >= MAX_CONCURRENT_TASKS) {
                    taskQueue.front().get();
                    taskQueue.pop();
                    std::cout << "One chunk task has ended" << std::endl;
                    currentTasks--;
                }

                taskQueue.push(std::async(std::launch::async, &Chunk::generateChunkData, &chunk));
                currentTasks++;
            }

            while (!taskQueue.empty())
            {
                taskQueue.front().get();
                taskQueue.pop();
                std::cout << "One chunk task has ended" << std::endl;
            }
            
            std::cout << "Generating rendering chunk data..." << std::endl;
            for (Chunk& chunk : chunks)
                chunk.generateRenderObject();
            std::cout << "All done!" << std::endl;

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
    // World transformation
    C3D_Mtx world, projection, viewModel, WVP;
    Mtx_Identity(&world);
    Mtx_Scale(&world, 1.0f, 1.0f, 1.0f);
    Mtx_Translate(&world, 0.0f, 0.0f, 0.0f, true);
    Mtx_RotateY(&world, C3D_AngleFromDegrees(0.0f), true);

    // Calculate the camera view matrix
    //Mtx_Identity(&view);
    //Mtx_RotateX(&view, C3D_AngleFromDegrees(camera.pitch), true);
    //Mtx_RotateY(&view, C3D_AngleFromDegrees(camera.yaw), false);
    //Mtx_Translate(&view, -camera.position.x, -camera.position.y, -camera.position.z, true);

    // Compute the projection matrix
	Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

    Mtx_Multiply(&viewModel, &camera.view, &world);
    Mtx_Multiply(&WVP, &projection, &viewModel);
    
    // Update uniforms 
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &WVP);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &world);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_material, &material);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec,     0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightHalfVec, 0.0f, 1.0f, 0.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);

    for (Chunk& chunk: chunks)
    {
        chunk.renderChunk();
    }
}