#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "render3d.h"
#include "render_utils.h"
#include "chunk.h"
#include "string_utils.h"
#include "dynalist.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static C2D_TextBuf g_textBuffer;
static C2D_Text g_fpsText[3];

static DynaList chunks;

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

    R3D_Init();
    if (!R3D_LoadShader("romfs:/shaders/chunk.shbin"))
    {
        printf("Failed to load shader\n");
        goto ERROR;
    }
    RScene mainScene;
    R3D_SceneInit(&mainScene, target3D);
    R3D_SceneSet(&mainScene);
    RCamera *actorCamera = R3D_GetCurrentCamera();
    C3D_Tex stoneTex;
    if (!loadTexture(&stoneTex, "romfs:/assets/debug/stone.t3x"))
    {
        printf("Failed to load texture\n");
        goto ERROR;
    }

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
        Chunk* currentChunk = DListGet(chunks, i);
        Chunk* neighbor;
        int x = i / 4;
        int z = i % 4;
        if (x > 0)
        {
            neighbor = DListGet(chunks, (x - 1) * 4 + z);
            currentChunk->left_neighbor = neighbor;
        }
        if (x < 3)
        {
            neighbor = DListGet(chunks, (x + 1) * 4 + z);
            currentChunk->right_neighbor = neighbor;
        }
        if (z > 0)
        {
            neighbor = DListGet(chunks, x * 4 + z - 1);
            currentChunk->back_neighbor = neighbor;
        }
        if (z < 3)
        {
            neighbor = DListGet(chunks, x * 4 + z + 1);
            currentChunk->front_neighbor = neighbor;
        }
        ChunkGenerateTerrain(currentChunk, 2342);
        printf("Chunk %d terrain data generated\n", i+1);
    }

    printf("Generating chunk mesh data...\n");
    for (int i = 0; i < 16; i++)
    {
        ChunkGenerateRenderObject(DListGet(chunks, i));
        printf("Chunk %d mesh data generated\n", i+1);
    }
    printf("All done!\n");

    struct timespec lastTime, endTime;
    double elapsed;
    float fps = 0.0f;
    sstring fpsString = sfromcstr("FPS: 0");
    sstring coordsString = sfromcstr("0, 0, 0");

    circlePosition cStick;

    float sensitivility = 0.05f;
    float speed = 0.3f;

    while (aptMainLoop())
    {
        clock_gettime(CLOCK_MONOTONIC, &lastTime);

        hidCstickRead(&cStick);
        hidScanInput();

        u32 kHeld = hidKeysHeld();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        // Camera movement
        R3D_RotateCamera(-cStick.dy * sensitivility, -cStick.dx * sensitivility);

        if (kHeld & KEY_UP)
            R3D_CameraMoveForward(-speed);
        if (kHeld & KEY_DOWN)
            R3D_CameraMoveForward(speed);
        if (kHeld & KEY_RIGHT)
            R3D_CameraMoveSide(-speed);
        if (kHeld & KEY_LEFT)
            R3D_CameraMoveSide(speed);
        if (kHeld & KEY_A)
            R3D_CameraMoveUpDown(speed);
        if (kHeld & KEY_B)
            R3D_CameraMoveUpDown(-speed);

        // Render the 3D scene
        R3D_SceneBegin();
        C3D_TexBind(0, &stoneTex);

        for (int i = 0; i < 16; i++)
            ChunkRender(DListGet(chunks, i), actorCamera);

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
        sassignformat(coordsString, "%.2f, %.2f, %.2f", actorCamera->position.x, actorCamera->position.y, actorCamera->position.z);
    }
    goto EXIT;

    ERROR:
    while (aptMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;
    }
    goto END;

    EXIT:
    for (int i = 0; i < 16; i++)
    {
        ChunkDestroy(DListGet(chunks, i));
        printf("Chunk %d deleted\n", i);
    }

    // Free global render resources
    R3D_Finish();

    // Free strings
    sdestroy(fpsString);
    sdestroy(coordsString);

    // Free lists
    DListFree(chunks);

    // Delete citro2d text buffers
    C2D_TextBufDelete(g_textBuffer);

    END:
	// Deinitialize graphics
	C3D_Fini();
    C2D_Fini();
	gfxExit();
    romfsExit();

    return 0;
}