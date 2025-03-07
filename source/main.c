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

#define TERRAIN_SIZE 6

static C2D_TextBuf g_textBuffer;
static C2D_Text g_fpsText[3];

static DynaList chunks;

int main()
{
    cfguInit(); // Allows system functions to work
    romfsInit();

    R3D_Init();
    consoleInit(GFX_BOTTOM, NULL);

    RScene mainScene;
    R3D_SceneInit(&mainScene);
    R3D_SceneSet(&mainScene);
    RCamera *actorCamera = R3D_GetCurrentCamera();

    if (!R3D_LoadShader("romfs:/shaders/chunk.shbin"))
    {
        printf("Failed to load shader\n");
        goto ERROR;
    }

    C3D_Tex stoneTex;
    if (!loadTexture(&stoneTex, "romfs:/assets/debug/stone.t3x"))
    {
        printf("Failed to load texture\n");
        goto ERROR;
    }

    DListInit(&chunks, sizeof(Chunk));
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            Chunk* currentChunk = DListAppendNew(chunks);
            ChunkInitDefault(currentChunk);
            currentChunk->x = i;
            currentChunk->z = j;
        }
    }
    printf("Chunks initialized\n");

    printf("Generating chunk data...\n");
    for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
    {
        Chunk* currentChunk = DListGet(chunks, i);
        Chunk* neighbor;
        int x = i / TERRAIN_SIZE;
        int z = i % TERRAIN_SIZE;
        if (x > 0)
        {
            neighbor = DListGet(chunks, (x - 1) * TERRAIN_SIZE + z);
            currentChunk->left_neighbor = neighbor;
        }
        if (x < TERRAIN_SIZE - 1)
        {
            neighbor = DListGet(chunks, (x + 1) * TERRAIN_SIZE + z);
            currentChunk->right_neighbor = neighbor;
        }
        if (z > 0)
        {
            neighbor = DListGet(chunks, x * TERRAIN_SIZE + z - 1);
            currentChunk->back_neighbor = neighbor;
        }
        if (z < TERRAIN_SIZE - 1)
        {
            neighbor = DListGet(chunks, x * TERRAIN_SIZE + z + 1);
            currentChunk->front_neighbor = neighbor;
        }
        ChunkGenerateTerrain(currentChunk, 2342);
        printf("Chunk %d terrain data generated\n", i+1);
    }

    printf("Generating chunk mesh data...\n");
    for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
    {
        ChunkGenerateRenderObject(DListGet(chunks, i));
        printf("Chunk %d mesh data generated\n", i+1);
    }
    printf("All done!\n");

    g_textBuffer = C2D_TextBufNew(4096);

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

        R3D_FrameBegin(); // Start frame
        // Render the 3D scene
        R3D_Scene3DBegin();
        C3D_TexBind(0, &stoneTex);

        for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
            ChunkRender(DListGet(chunks, i), actorCamera);

        // Render the 2D scene
        R3D_Scene2DBegin();

        C2D_TextBufClear(g_textBuffer);

        C2D_TextParse(&g_fpsText[0], g_textBuffer, fpsString->cstr);
        C2D_TextOptimize(&g_fpsText[0]);
        C2D_DrawText(&g_fpsText[0], C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));

        C2D_TextParse(&g_fpsText[1], g_textBuffer, coordsString->cstr);
        C2D_TextOptimize(&g_fpsText[1]);
        C2D_DrawText(&g_fpsText[1], C2D_WithColor, 10.0f, 220.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1.0, 1.0, 1.0, 1.0));
        
		R3D_FrameEnd(); // End frame

        // Calculate frame time
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        elapsed = (endTime.tv_sec - lastTime.tv_sec) + (endTime.tv_nsec - lastTime.tv_nsec) / 1000000000.0f;

        fps = 1.0f / elapsed;

        sassignformat(fpsString, "FPS: %.2f", fps);
        sassignformat(coordsString, "%.2f, %.2f, %.2f", actorCamera->position.x, actorCamera->position.y, actorCamera->position.z);
    }
    goto EXIT;

    ERROR:
    printf("\nPress Start to exit");
    while (aptMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;
    }
    goto END;

    EXIT:
    for (int i = 0; i < TERRAIN_SIZE * TERRAIN_SIZE; i++)
    {
        ChunkDestroy(DListGet(chunks, i));
        printf("Chunk %d deleted\n", i);
    }
    // Free strings
    sdestroy(fpsString);
    sdestroy(coordsString);

    // Free lists
    DListFree(chunks);

    // Delete citro2d text buffers
    C2D_TextBufDelete(g_textBuffer);

    END:
    // Free global render resources
    R3D_Release();
	// Deinitialize graphics
    R3D_SceneDelete(&mainScene);
    R3D_Fini();
	gfxExit();
    romfsExit();

    return 0;
}