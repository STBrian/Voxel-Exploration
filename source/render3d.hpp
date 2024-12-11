#pragma once

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

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

        bool loadShader(std::string shaderFp)
        {
            bool result = false;
            std::cout << "Loading shader at directory: " << shaderFp << std::endl;
            std::ifstream file(shaderFp, std::ios::binary | std::ios::ate);
            if (!file) 
                std::cout << "Failed to open shader file" << std::endl;
            else
            {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                uint8_t *buffer = new uint8_t[size];
                if (!buffer)
                    std::cout << "Unable to allocate memory for shader buffer file" << std::endl;
                else
                {
                    if (!file.read(reinterpret_cast<char *>(buffer), size)) 
                        std::cout << "Unable to read the shader file" << std::endl;
                    else
                    {
                        this->vshader_dvlb = DVLB_ParseFile(reinterpret_cast<uint32_t *>(buffer), size / 4);
                        if (this->vshader_dvlb == nullptr)
                            std::cout << "Unable to parse the shader" << std::endl;
                        else
                        {
                            Result r1 = shaderProgramInit(&this->program);
                            Result r2 = shaderProgramSetVsh(&this->program, &vshader_dvlb->DVLE[0]);
                            if (r1 || r2)
                                std::cout << "Unable to init the shader" << std::endl;
                            else
                            {
                                // Get the location of the uniforms
                                this->uLoc_projection   = shaderInstanceGetUniformLocation(this->program.vertexShader, "projection");
                                this->uLoc_modelView    = shaderInstanceGetUniformLocation(this->program.vertexShader, "modelView");
                                this->uLoc_lightVec     = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightVec");
                                this->uLoc_lightHalfVec = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightHalfVec");
                                this->uLoc_lightClr     = shaderInstanceGetUniformLocation(this->program.vertexShader, "lightClr");
                                this->uLoc_material     = shaderInstanceGetUniformLocation(this->program.vertexShader, "material");

                                result = true;
                            }
                        }
                    }
                }
                delete[] buffer;
            }
            return result;
        }

        void initDefaults()
        {
            Mtx_Identity(&this->world);
            Mtx_Identity(&this->modelView);

            Mtx_PerspTilt(&this->projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
        }

        void freeShaderProgram()
        {
            shaderProgramFree(&this->program);
            DVLB_Free(this->vshader_dvlb);
        }

        void setWorldViewMatrix(C3D_Mtx *worldMtx)
        {
            Mtx_Copy(&this->world, worldMtx);
        }

        void setModelViewMatrix(C3D_Mtx *in)
        {
            Mtx_Copy(&this->modelView, in);
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
            AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 1); // v2=normal

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
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_modelView, &this->modelView);
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
        C3D_Mtx world, projection, cameraView, modelView;
};