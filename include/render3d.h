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
} RScene;

struct shader_uniforms_s {
    int uLoc_projection;
    int uLoc_modelView;
    int uLoc_lightVec;
    int uLoc_lightHalfVec;
    int uLoc_lightClr;
    int uLoc_material;
    int uLoc_textureProp;
};

typedef struct g_render3d {
    uint32_t *shaderData;
    DVLB_s *vshaderDvlb;
    shaderProgram_s *shaderProgram;
    struct shader_uniforms_s shader_uniforms;

    RScene *scene;

    C3D_Mtx mtx_material;
    C3D_Mtx mtx_world;
    C3D_Mtx mtx_projection;
    C3D_Mtx mtx_cameraView;
    C3D_Mtx mtx_modelView;

    u32 clearColor;
    bool printDebug;
} RRender3D;

extern RRender3D *GlobalRender;

typedef struct g_cubic_instance {
    CompressedVertex *vbo;
    uint16_t *ibo;
    unsigned int faces_count;
    unsigned int vtx_count;
    unsigned int idx_count;
} CubicInstance;

void R3D_Init();

void R3D_Fini();

bool R3D_LoadShader(const char *shaderFp);

void R3D_FreeShaderProgram();

void R3D_SceneInit(RScene *scene);

void R3D_SceneDelete(RScene *scene);




void R3D_SceneSet(RScene *scene);

void R3D_FrameBegin();

void R3D_Scene3DBegin();

void R3D_Scene2DBegin();

void R3D_FrameEnd();

void R3D_DrawCubicInstance(CubicInstance *instance);

void R3D_Release();




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