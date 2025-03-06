#include "render3d.h"

#include <stdio.h>

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define M_PI_2 1.57079632679489661923

RRender3D* GlobalRender = NULL;

/*

    The following functions uses GlobalRender to share state info between calls

*/

void R3D_Init()
{
    // Initialize graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

    GlobalRender = (RRender3D*)malloc(sizeof(RRender3D));
    C3D_Mtx material = {
        {
        { { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
        { { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
        { { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
        { { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
        }
    };
    GlobalRender->mtx_material = material;
    Mtx_Identity(&GlobalRender->mtx_world);
    Mtx_Identity(&GlobalRender->mtx_modelView);

    Mtx_PerspTilt(&GlobalRender->mtx_projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false); 

    GlobalRender->shaderProgram = NULL;
    GlobalRender->vshaderDvlb = NULL;
    GlobalRender->shaderData = NULL;

    GlobalRender->scene = NULL;
    GlobalRender->clearColor = 0x000000FF;

    GlobalRender->printDebug = true;
}

void R3D_Fini()
{
    C2D_Fini();
    C3D_Fini();
}

void R3D_SceneInit(RScene *scene)
{
    // Initialize the render target
	C3D_RenderTarget* target3D = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target3D, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    scene->target = target3D;
    scene->camera.position = FVec3_New(0.0f, 70.0f, 0.0f);
    scene->camera.pitch = 0.0f;
    scene->camera.yaw = 180.0f;
    scene->camera.pitchMin = -89.0f;
    scene->camera.pitchMax = 89.0f;
}

void R3D_SceneDelete(RScene *scene)
{
    C3D_RenderTargetDelete(scene->target);
}


bool R3D_LoadShader(const char* shaderFp)
{
    bool result = false;
    if (GlobalRender->printDebug)
        printf("Loading shader at directory: %s\n", shaderFp);        
    FILE* file = fopen(shaderFp, "rb");
    if (!file)
    {
        if (GlobalRender->printDebug)
        {
            printf("Failed to open shader file\n");
        }
    }
    else
    {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        GlobalRender->shaderData = (uint32_t*)malloc(sizeof(uint32_t)* size/4);
        if (GlobalRender->shaderData == NULL)
        {
            if (GlobalRender->printDebug)
            {
                printf("Unable to allocate memory for shader buffer file\n");
            }
        }
        else
        {
            if (fread(GlobalRender->shaderData, sizeof(uint32_t), size/4, file) < size/4)
            {
                if (GlobalRender->printDebug)
                {
                    printf("Unable to read the shader file\n");
                }
            }
            else
            {
                GlobalRender->vshaderDvlb = DVLB_ParseFile(GlobalRender->shaderData, size/4);
                if (GlobalRender->vshaderDvlb == NULL)
                {
                    if (GlobalRender->printDebug)
                    {
                        printf("Unable to parse the shader\n");
                    }
                }
                else
                {
                    GlobalRender->shaderProgram = malloc(sizeof(shaderProgram_s));
                    if (GlobalRender->shaderProgram)
                    {
                        Result r1 = shaderProgramInit(GlobalRender->shaderProgram);
                        Result r2 = shaderProgramSetVsh(GlobalRender->shaderProgram, &GlobalRender->vshaderDvlb->DVLE[0]);
                        if (r1 || r2)
                        {
                            if (GlobalRender->printDebug)
                            {
                                printf("Unable to parse the shader\n");
                            }
                        }
                        else
                        {
                            // Get the location of the uniforms
                            GlobalRender->shader_uniforms.uLoc_projection   = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "projection");
                            GlobalRender->shader_uniforms.uLoc_modelView    = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "modelView");
                            GlobalRender->shader_uniforms.uLoc_lightVec     = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "lightVec");
                            GlobalRender->shader_uniforms.uLoc_lightHalfVec = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "lightHalfVec");
                            GlobalRender->shader_uniforms.uLoc_lightClr     = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "lightClr");
                            GlobalRender->shader_uniforms.uLoc_material     = shaderInstanceGetUniformLocation(GlobalRender->shaderProgram->vertexShader, "material");

                            result = true;
                        }
                    }
                }
            }
        }
        fclose(file);
        //free(buffer); // Note: Do not free the buffer >:c
    }
    return result;
}

void R3D_FreeShaderProgram()
{
    shaderProgramFree(GlobalRender->shaderProgram);
    free(GlobalRender->shaderProgram);
    DVLB_Free(GlobalRender->vshaderDvlb);
    free(GlobalRender->shaderData);
    GlobalRender->shaderProgram = NULL;
    GlobalRender->vshaderDvlb = NULL;
    GlobalRender->shaderData = NULL;
}

void R3D_SceneSet(RScene *scene)
{
    GlobalRender->scene = scene;
}

void R3D_Scene3DBegin()
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(GlobalRender->scene->target, C3D_CLEAR_ALL, GlobalRender->clearColor, 0);
    C3D_FrameDrawOn(GlobalRender->scene->target);
    C3D_BindProgram(GlobalRender->shaderProgram);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_UNSIGNED_BYTE, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 1); // v2=normal

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

    C3D_CullFace(GPU_CULL_BACK_CCW);

    // Set uniforms
    C3D_FVUnifSet(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_lightVec,     0.0f, 1.0f, 0.0f, 0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_lightHalfVec, 0.0f, -1.0f, 0.0f, 0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);
}

void R3D_Scene2DBegin()
{
    C2D_Prepare();
    C2D_SceneBegin(GlobalRender->scene->target);
}

void R3D_FrameEnd()
{
    C3D_FrameEnd(0);
}

void R3D_DrawCubicInstance(CubicInstance *instance)
{
    C3D_Mtx viewModel, WVP;
    Mtx_Multiply(&viewModel, &GlobalRender->mtx_cameraView, &GlobalRender->mtx_world);
    Mtx_Multiply(&WVP, &GlobalRender->mtx_projection, &viewModel);

    // Update uniforms 
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_projection, &WVP);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_modelView, &GlobalRender->mtx_modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, GlobalRender->shader_uniforms.uLoc_material, &GlobalRender->mtx_material);

    C3D_BufInfo *bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, instance->vbo, sizeof(CompressedVertex), 3, 0x210);

    C3D_DrawElements(GPU_TRIANGLES, instance->idx_count, C3D_UNSIGNED_SHORT, instance->ibo);
}

void R3D_Finish()
{
    if (GlobalRender != NULL)
    {
        R3D_FreeShaderProgram();
        free(GlobalRender);
        GlobalRender = NULL;
    }
}




RCamera *R3D_GetCurrentCamera()
{
    return &GlobalRender->scene->camera;
}

void R3D_RotateCamera(float pitch, float yaw)
{
    GlobalRender->scene->camera.pitch += pitch;
    GlobalRender->scene->camera.yaw += yaw;
    if (GlobalRender->scene->camera.pitch > GlobalRender->scene->camera.pitchMax) GlobalRender->scene->camera.pitch = GlobalRender->scene->camera.pitchMax;
    if (GlobalRender->scene->camera.pitch < GlobalRender->scene->camera.pitchMin) GlobalRender->scene->camera.pitch = GlobalRender->scene->camera.pitchMin;
    if (GlobalRender->scene->camera.yaw > 360.0f) GlobalRender->scene->camera.yaw -= 360.0f;
    if (GlobalRender->scene->camera.yaw < 0.0f) GlobalRender->scene->camera.yaw += 360.0f;

    float yawRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.yaw);
    float pitchRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.pitch);

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

    R3D_SetCameraViewMatrx(GlobalRender->scene->camera.position, FVec3_Negate(FVec3_Add(FVec3_Negate(GlobalRender->scene->camera.position), forward)), FVec3_Negate(up));
}

void R3D_CameraMoveForward(float value)
{
    float yawRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.yaw);
    float pitchRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.pitch);

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

    GlobalRender->scene->camera.position = FVec3_Add(GlobalRender->scene->camera.position, FVec3_Scale(forwardXZ, value));
    R3D_SetCameraViewMatrx(GlobalRender->scene->camera.position, FVec3_Negate(FVec3_Add(FVec3_Negate(GlobalRender->scene->camera.position), forward)), FVec3_Negate(up));
}

void R3D_CameraMoveSide(float value)
{
    float yawRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.yaw);
    float pitchRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.pitch);

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

    GlobalRender->scene->camera.position = FVec3_Add(GlobalRender->scene->camera.position, FVec3_Scale(right, value));
    R3D_SetCameraViewMatrx(GlobalRender->scene->camera.position, FVec3_Negate(FVec3_Add(FVec3_Negate(GlobalRender->scene->camera.position), forward)), FVec3_Negate(up));
}

void R3D_CameraMoveUpDown(float value)
{
    float yawRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.yaw);
    float pitchRad = C3D_AngleFromDegrees(GlobalRender->scene->camera.pitch);

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

    GlobalRender->scene->camera.position = FVec3_Add(GlobalRender->scene->camera.position, FVec3_New(0.0f, value, 0.0f));
    R3D_SetCameraViewMatrx(GlobalRender->scene->camera.position, FVec3_Negate(FVec3_Add(FVec3_Negate(GlobalRender->scene->camera.position), forward)), FVec3_Negate(up));
}




void R3D_SetWorldViewMatrix(C3D_Mtx* worldMtx)
{
    Mtx_Copy(&GlobalRender->mtx_world, worldMtx);
}

void R3D_SetModelViewMatrx(C3D_Mtx* in)
{
    Mtx_Copy(&GlobalRender->mtx_modelView, in);
}

C3D_Mtx *R3D_GetWorldMatrx()
{
    return &GlobalRender->mtx_world;
}

void R3D_SetProjectionMatrix(C3D_Mtx* projMtx)
{
    Mtx_Copy(&GlobalRender->mtx_projection, projMtx);
}

void R3D_SetCameraViewMatrx(C3D_FVec position, C3D_FVec target, C3D_FVec up)
{
    Mtx_LookAt(&GlobalRender->mtx_cameraView, position, target, up, false);
}