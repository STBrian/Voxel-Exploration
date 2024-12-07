#pragma once

#include <fstream>
#include <cstdint>

#include <3ds.h>
#include <citro3d.h>

#include "geometry.hpp"

typedef struct {
    C3D_FVec position;
    float yaw;
    float pitch;
    C3D_Mtx view;
} Camera;

class Render3D 
{
    public:
        static Render3D& getInstance()
        {
            static Render3D instance;
            return instance;
        }

        bool loadResources()
        {
            std::ifstream file("romfs:/shaders/full_block.shbin", std::ios::binary | std::ios::ate);
            if (file) {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                uint8_t *buffer = new uint8_t[size];
                if (file.read(reinterpret_cast<char *>(buffer), size)) {
                    vshader_dvlb = DVLB_ParseFile(reinterpret_cast<uint32_t *>(buffer), size / 4);
                    shaderProgramInit(&this->program);
                    shaderProgramSetVsh(&this->program, &vshader_dvlb->DVLE[0]);

                    // Get the location of the uniforms
                    this->uLoc_projection   = shaderInstanceGetUniformLocation(this->program.vertexShader, "projection");
                    this->uLoc_modelView    = shaderInstanceGetUniformLocation(this->program.vertexShader, "modelView");
                    this->uLoc_lightVec     = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightVec");
                    this->uLoc_lightHalfVec = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightHalfVec");
                    this->uLoc_lightClr     = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightClr");
                    this->uLoc_material     = shaderInstanceGetUniformLocation(this->program.vertexShader, "material");

                    Mtx_Identity(&this->world);
                    Mtx_Scale(&this->world, 1.0f, 1.0f, 1.0f);
                    Mtx_RotateY(&this->world, C3D_AngleFromDegrees(0.0f), true);
                    Mtx_Translate(&this->world, 0.0f, 0.0f, 0.0f, true);

                    Mtx_PerspTilt(&this->projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

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

        void setWorldViewMatrix(C3D_Mtx *worldMtx)
        {
            Mtx_Copy(&this->world, worldMtx);
        }

        C3D_Mtx *getWorldMatrix()
        {
            return &this->world;
        }

        void setProjectionMatrix(C3D_Mtx *projMtx)
        {
            Mtx_Copy(&this->projection, projMtx);
        }

        void setCameraViewMatrix(C3D_FVec position, C3D_FVec target, C3D_FVec up)
        {
            Mtx_LookAt(&this->cameraView, position, target, up, false);
        }

        void renderOptimizedInstance(CompressedVertex *vbo, uint16_t *ibo, int count)
        {
            C3D_BindProgram(&this->program);

            C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
            AttrInfo_Init(attrInfo);
            AttrInfo_AddLoader(attrInfo, 0, GPU_UNSIGNED_BYTE, 3); // v0=position
            AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
            AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 3); // v2=normal

            C3D_TexBind(0, NULL);

            C3D_TexEnv* env = C3D_GetTexEnv(0);
            C3D_TexEnvInit(env);
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

            C3D_CullFace(GPU_CULL_BACK_CCW);

            C3D_Mtx viewModel, WVP;
            Mtx_Multiply(&viewModel, &this->cameraView, &this->world);
            Mtx_Multiply(&WVP, &this->projection, &viewModel);

            // Update uniforms 
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_projection, &WVP);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_modelView, &this->world);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_material, &this->material);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightVec,     0.0f, -1.0f, 0.0f, 0.0f);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightHalfVec, 0.0f, 1.0f, 0.0f, 0.0f);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightClr,     1.0f, 1.0f,  1.0f, 1.0f);

            C3D_BufInfo *bufInfo = C3D_GetBufInfo();
            BufInfo_Init(bufInfo);
            BufInfo_Add(bufInfo, vbo, sizeof(CompressedVertex), 3, 0x210);

            C3D_DrawElements(GPU_TRIANGLES, count, C3D_UNSIGNED_SHORT, ibo);
        }

        void setLocModelView(int loc)
        {
            this->uLoc_modelView = loc;
        }

        int getLocModelView()
        {
            return this->uLoc_modelView;
        }

    private:
        Render3D() {
            material =
            {
                {
                { { 0.0f, 0.2f, 0.2f, 0.2f } }, // Ambient
                { { 0.0f, 0.4f, 0.4f, 0.4f } }, // Diffuse
                { { 0.0f, 0.8f, 0.8f, 0.8f } }, // Specular
                { { 1.0f, 0.0f, 0.0f, 0.0f } }, // Emission
                }
            };
        }

        ~Render3D() 
        {
            shaderProgramFree(&program);
            DVLB_Free(vshader_dvlb);
        }

        DVLB_s* vshader_dvlb;
        shaderProgram_s program;
        int uLoc_projection, uLoc_modelView;
        int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
        C3D_Mtx material;
        C3D_Mtx world, projection, cameraView;
};