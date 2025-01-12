#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "render3d.h"
#include "chunk.h"
#include "string_utils.h"
#include "dynalist.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define M_PI_2		1.57079632679489661923

static C2D_TextBuf g_textBuffer;
static C2D_Text g_fpsText[3];

static DynaList chunks;
static RCamera camera;

int main()
{
    cfguInit(); // Allows system functions to work
    romfsInit();

    // Initialize graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

    // Initialize the render target
	C3D_RenderTarget* target3D = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target3D, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    consoleInit(GFX_BOTTOM, NULL);

    g_textBuffer = C2D_TextBufNew(4096);

    grenderPrepare();
    bool success = gloadShader("romfs:/shaders/chunk.shbin");

    bool chunkDataGenerated = true;

    DListInit(&chunks, sizeof(Chunk));
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            Chunk* currentChunk = DListAppendNew(chunks);
            ChunkInitDefault(currentChunk);
            currentChunk->x = i;
            currentChunk->z = j;
        }
    }
    printf("Chunks initialized\n");

    printf("Generating chunk data...\n");
    for (int i = 0; i < 16; i++)
    {
        ChunkGenerateTerrain(DListGet(chunks, i), 2342);
        printf("Chunk %d terrain data generated\n", i+1);
    }

    printf("Generating chunk mesh data...\n");
    for (int i = 0; i < 16; i++)
    {
        ChunkGenerateRenderObject(DListGet(chunks, i));
        printf("Chunk %d mesh data generated\n", i+1);
    }
    chunkDataGenerated = true;
    printf("All done!\n");

    struct timespec lastTime, endTime;
    double elapsed;
    float fps = 0.0f;
    sstring fpsString = sfromcstr("FPS: 0");
    sstring coordsString = sfromcstr("0, 0, 0");

    camera.position = FVec3_New(0.0f, 70.0f, 0.0f);
    camera.yaw = 0.0f;
    camera.pitch = 0.0f;

    circlePosition cStick;

    int clearColor = 0x000000FF;
    float sensitivility = 0.05f;
    float speed = 0.3f;

    bool loadedSuccess = success && chunkDataGenerated;

    while (aptMainLoop() && loadedSuccess)
    {
        clock_gettime(CLOCK_MONOTONIC, &lastTime);

        hidCstickRead(&cStick);
        hidScanInput();

        u32 kHeld = hidKeysHeld();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

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
            camera.position = FVec3_Subtract(camera.position, FVec3_Scale(forwardXZ, speed));
        if (kHeld & KEY_DOWN)
            camera.position = FVec3_Add(camera.position, FVec3_Scale(forwardXZ, speed));
        if (kHeld & KEY_RIGHT)
            camera.position = FVec3_Subtract(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_LEFT)
            camera.position = FVec3_Add(camera.position, FVec3_Scale(right, speed));
        if (kHeld & KEY_A)
            camera.position = FVec3_Add(camera.position, FVec3_New(0.0f, speed, 0.0f));
        if (kHeld & KEY_B)
            camera.position = FVec3_Subtract(camera.position, FVec3_New(0.0f, speed, 0.0f));

        gsetCameraViewMatrx(camera.position, FVec3_Negate(FVec3_Add(FVec3_Negate(camera.position), forward)), FVec3_Negate(up));

        // Render the 3D scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target3D, C3D_CLEAR_ALL, clearColor, 0);
        C3D_FrameDrawOn(target3D);

        for (int i = 0; i < 16; i++)
            ChunkRender(DListGet(chunks, i), &camera);

        // Render the 2D scene
        C2D_Prepare();
        C2D_SceneBegin(target3D);

        C2D_TextBufClear(g_textBuffer);

        C2D_TextParse(&g_fpsText[0], g_textBuffer, fpsString->cstr);
        C2D_TextOptimize(&g_fpsText[0]);
        C2D_DrawText(&g_fpsText[0], C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));

        C2D_TextParse(&g_fpsText[1], g_textBuffer, coordsString->cstr);
        C2D_TextOptimize(&g_fpsText[1]);
        C2D_DrawText(&g_fpsText[1], C2D_WithColor, 10.0f, 220.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));
        
		C3D_FrameEnd(0);

        // Calculate frame time
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        elapsed = (endTime.tv_sec - lastTime.tv_sec) + (endTime.tv_nsec - lastTime.tv_nsec) / 1000000000.0f;

        fps = 1.0f / elapsed;

        sassignformat(fpsString, "FPS: %.2f", fps);
        sassignformat(coordsString, "%.2f, %.2f, %.2f", camera.position.x, camera.position.y, camera.position.z);
    }

    for (int i = 0; i < 16; i++)
    {
        ChunkDestroy(DListGet(chunks, i));
        printf("Chunk %d deleted\n", i);
    }

    // Free global render resources
    gfreeShaderProgram();
    grenderDestroy();

    // Free strings
    sdestroy(fpsString);
    sdestroy(coordsString);

    // Free lists
    DListFree(chunks);

    // Delete citro2d text buffers
    C2D_TextBufDelete(g_textBuffer);

	// Deinitialize graphics
	C3D_Fini();
    C2D_Fini();
	gfxExit();
    romfsExit();

    return 0;
}