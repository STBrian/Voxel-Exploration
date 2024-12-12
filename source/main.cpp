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

#include "render3d.hpp"
#include "chunk.hpp"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

C2D_TextBuf g_textBuffer;
C2D_Text g_fpsText;

std::vector<Chunk> chunks;
static Camera camera;

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

    std::string shaderFp = "romfs:/shaders/full_block.shbin";

    bool success = Render3D::getInstance().loadShader(shaderFp);
    Render3D::getInstance().initDefaults();

    std::cout << "Generating chunk data..." << std::endl;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            chunks.emplace_back(i, j);

    for (Chunk& chunk : chunks)
    {
        chunk.generateChunkData();
        std::cout << "One chunk task has ended" << std::endl;
    }

    std::cout << "Generating rendering chunk data..." << std::endl;
    for (Chunk& chunk : chunks)
    {
        chunk.generateRenderObject();
        std::cout << "One chunk task has ended" << std::endl;
    }
    std::cout << "All done!" << std::endl;

    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
    float fps = 0.0f;
    std::string fpsString = "FPS: 0";

    int clearColor = 0x000000FF;
    float sensitivility = 0.05f;
    float speed = 0.3f;

    bool reloadShader = true;

    // Main loop
	while (aptMainLoop() && success)
	{
        if (reloadShader)
        {
            Render3D::getInstance().freeShaderProgram();
            Render3D::getInstance().loadShader(shaderFp);
            reloadShader = false;
        }
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
        camera.yaw -= cStick.dx * sensitivility;
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
            camera.position = FVec3_Add(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_LEFT)
            camera.position = FVec3_Subtract(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_A)
            camera.position = FVec3_Subtract(camera.position, FVec3_New(0.0f, speed, 0.0f));
        if (kHeld & KEY_B)
            camera.position = FVec3_Add(camera.position, FVec3_New(0.0f, speed, 0.0f));
        
        Render3D::getInstance().setCameraViewMatrix(FVec3_Negate(camera.position), FVec3_Negate(FVec3_Add(camera.position, forward)), FVec3_Negate(up));

        // Render the 3D scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target3D, C3D_CLEAR_ALL, clearColor, 0);
        C3D_FrameDrawOn(target3D);
        
        for (Chunk& chunk: chunks)
            chunk.renderChunk();

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

        if (kDown & KEY_SELECT)
        {
            std::cout << "Generating rendering chunk data..." << std::endl;
            for (Chunk& chunk : chunks)
            {
                chunk.freeRenderObject();
                chunk.generateRenderObject();
                std::cout << "One chunk task has ended" << std::endl;
            }
            std::cout << "All done!" << std::endl;
            reloadShader = true;
        }
	}

    for (Chunk& chunk : chunks)
        chunk.freeRenderObject();

	// Free the shader program
    //Render3D::getInstance().freeShaderProgram();

    C2D_TextBufDelete(g_textBuffer);

	// Deinitialize graphics
	C3D_Fini();
    C2D_Fini();
	gfxExit();
    romfsExit();

    return 0;
}