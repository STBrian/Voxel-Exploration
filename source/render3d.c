#include "render3d.h"

#include <stdio.h>

RRender3D* GlobalRender = NULL;

void rrenderInit(RRender3D* render_obj)
{
    C3D_Mtx material = {
        {
        { { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
        { { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
        { { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
        { { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
        }
    };
    render_obj->material = material;
    Mtx_Identity(&render_obj->world);
    Mtx_Identity(&render_obj->modelView);

    Mtx_PerspTilt(&render_obj->projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

    FILE *texFile = fopen("romfs:/textures/stone.t3x", "rb");
    Tex3DS_TextureImportStdio(texFile, &render_obj->blockTex, NULL, false);
    C3D_TexSetFilter(&render_obj->blockTex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&render_obj->blockTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    render_obj->printDebug = true;
}

void grenderPrepare()
{
    GlobalRender = (RRender3D*)malloc(sizeof(RRender3D));
    rrenderInit(GlobalRender);
}

void grenderDestroy()
{
    if (GlobalRender != NULL)
    {
        free(GlobalRender);
        GlobalRender = NULL;
    }
}

bool rloadShaderInInstance(RRender3D* render_obj, const char* shaderFp)
{
    bool result = false;
    if (render_obj->printDebug)
        printf("Loading shader at directory: %s\n", shaderFp);
    FILE* file = fopen(shaderFp, "rb");
    if (!file)
    {
        if (render_obj->printDebug)
        {
            printf("Failed to open shader file\n");
        }
    }
    else
    {
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t)*size);
        if (!buffer)
        {
            if (render_obj->printDebug)
            {
                printf("Unable to allocate memory for shader buffer file\n");
            }
        }
        else
        {
            if (fread(buffer, sizeof(uint8_t), size, file) < size)
            {
                if (render_obj->printDebug)
                {
                    printf("Unable to read the shader file\n");
                }
            }
            else
            {
                render_obj->vshader_dvlb = DVLB_ParseFile((uint32_t*)buffer, size / 4);
                if (render_obj->vshader_dvlb == NULL)
                {
                    if (render_obj->printDebug)
                    {
                        printf("Unable to parse the shader\n");
                    }
                }
                else
                {
                    Result r1 = shaderProgramInit(&render_obj->program);
                    Result r2 = shaderProgramSetVsh(&render_obj->program, &render_obj->vshader_dvlb->DVLE[0]);
                    if (r1 || r2)
                    {
                        if (render_obj->printDebug)
                        {
                            printf("Unable to parse the shader\n");
                        }
                    }
                    else
                    {
                        // Get the location of the uniforms
                        render_obj->uLoc_projection   = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "projection");
                        render_obj->uLoc_modelView    = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "modelView");
                        render_obj->uLoc_lightVec     = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "lightVec");
                        render_obj->uLoc_lightHalfVec = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "lightHalfVec");
                        render_obj->uLoc_lightClr     = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "lightClr");
                        render_obj->uLoc_material     = shaderInstanceGetUniformLocation(render_obj->program.vertexShader, "material");

                        result = true;
                    }
                }
            }
        }
        free(buffer);
    }
    return result;
}

bool gloadShader(const char* shaderFp)
{
    return rloadShaderInInstance(GlobalRender, shaderFp);
}

void rfreeShaderProgramInInstance(RRender3D* render_obj)
{
    shaderProgramFree(&render_obj->program);
    DVLB_Free(render_obj->vshader_dvlb);
}

void gfreeShaderProgram()
{
    rfreeShaderProgramInInstance(GlobalRender);
}

void gsetWorldViewMatrix(C3D_Mtx* worldMtx)
{
    Mtx_Copy(&GlobalRender->world, worldMtx);
}

void gsetModelViewMatrx(C3D_Mtx* in)
{
    Mtx_Copy(&GlobalRender->modelView, in);
}

C3D_Mtx* ggetWorldMatrx()
{
    return &GlobalRender->world;
}

void gsetProjectionMatrix(C3D_Mtx* projMtx)
{
    Mtx_Copy(&GlobalRender->projection, projMtx);
}

void gsetCameraViewMatrx(C3D_FVec position, C3D_FVec target, C3D_FVec up)
{
    Mtx_LookAt(&GlobalRender->cameraView, position, target, up, false);
}

void rrenderOptimizedInstance(RRender3D* render_obj, CompressedVertex* vbo, uint16_t* ibo, int count)
{
    C3D_BindProgram(&render_obj->program);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_UNSIGNED_BYTE, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 1); // v2=normal

    C3D_TexBind(0, &render_obj->blockTex);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

    C3D_CullFace(GPU_CULL_BACK_CCW);

    C3D_Mtx viewModel, WVP;
    Mtx_Multiply(&viewModel, &render_obj->cameraView, &render_obj->world);
    Mtx_Multiply(&WVP, &render_obj->projection, &viewModel);

    // Update uniforms 
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, render_obj->uLoc_projection, &WVP);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, render_obj->uLoc_modelView, &render_obj->modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, render_obj->uLoc_material, &render_obj->material);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, render_obj->uLoc_lightVec,     0.0f, 1.0f, 0.0f, 0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, render_obj->uLoc_lightHalfVec, 0.0f, -1.0f, 0.0f, 0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, render_obj->uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);

    C3D_BufInfo *bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo, sizeof(CompressedVertex), 3, 0x210);

    C3D_DrawElements(GPU_TRIANGLES, count, C3D_UNSIGNED_SHORT, ibo);
}

void grenderOptimizedInstance(CompressedVertex* vbo, uint16_t* ibo, int count)
{
    rrenderOptimizedInstance(GlobalRender, vbo, ibo, count);
}