#pragma once

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "geometry.h"

typedef struct g_camera {
    C3D_FVec position;
    float yaw;
    float pitch;
    float pitchMax;
    float pitchMin;
} RCamera;

typedef struct g_scene {
    C3D_RenderTarget *target;
    RCamera camera;
    u32 clearColor;
} RScene;

typedef struct g_render3d {
    uint32_t *shaderData;
    DVLB_s *vshaderDvlb;
    shaderProgram_s *shaderProgram;

    RScene *scene;

    int uLoc_projection;
    int uLoc_modelView;
    int uLoc_lightVec;
    int uLoc_lightHalfVec;
    int uLoc_lightClr;
    int uLoc_material;

    C3D_Mtx material;
    C3D_Mtx world;
    C3D_Mtx projection;
    C3D_Mtx cameraView;
    C3D_Mtx modelView;

    bool printDebug;
} RRender3D;

extern RRender3D *GlobalRender;


void R3D_Init();

bool R3D_LoadShader(const char *shaderFp);

void R3D_FreeShaderProgram();

void R3D_SceneInit(RScene *scene, C3D_RenderTarget *target);

void R3D_SceneSet(RScene *scene);

void R3D_SceneBegin();

void R3D_DrawOptimizedInstance(CompressedVertex *vbo, uint16_t *ibo, int count);

void R3D_Finish();




RCamera *R3D_GetCurrentCamera();

void R3D_RotateCamera(float pitch, float yaw);

void R3D_CameraMoveForward(float value);

void R3D_CameraMoveSide(float value);

void R3D_CameraMoveUpDown(float value);




void R3D_SetWorldViewMatrix(C3D_Mtx *worldMtx);

void R3D_SetModelViewMatrx(C3D_Mtx *in);

C3D_Mtx *R3D_GetWorldMatrx();

void R3D_SetProjectionMatrix(C3D_Mtx *projMtx);

void R3D_SetCameraViewMatrx(C3D_FVec position, C3D_FVec target, C3D_FVec up);