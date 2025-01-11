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
} RCamera;

typedef struct g_render3d {
    uint32_t* shaderData;
    DVLB_s* vshader_dvlb;
    shaderProgram_s program;
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
    C3D_Tex blockTex;
    bool printDebug;
} RRender3D;

extern RRender3D* GlobalRender;

void rrenderInit(RRender3D* render_obj);

bool rloadShaderInInstance(RRender3D* render_obj, const char* shaderFp);

void rfreeShaderProgramInInstance(RRender3D* render_obj);

void grenderPrepare();

void grenderDestroy();

bool gloadShader(const char* shaderFp);

void gfreeShaderProgram();

void gsetWorldViewMatrix(C3D_Mtx* worldMtx);

void gsetModelViewMatrx(C3D_Mtx* in);

C3D_Mtx* ggetWorldMatrx();

void gsetProjectionMatrix(C3D_Mtx* projMtx);

void gsetCameraViewMatrx(C3D_FVec position, C3D_FVec target, C3D_FVec up);

void rrenderOptimizedInstance(RRender3D* render_obj, CompressedVertex* vbo, uint16_t* ibo, int count);

void grenderOptimizedInstance(CompressedVertex* vbo, uint16_t* ibo, int count);