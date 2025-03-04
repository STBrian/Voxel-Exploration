#include "render_utils.h"

#include <3ds.h>
#include <tex3ds.h>

#include <stdio.h>

bool loadTexture(C3D_Tex *outTex, const char *fp)
{
    FILE *texFile = fopen(fp, "rb");
    if (texFile)
    {
        Tex3DS_TextureImportStdio(texFile, outTex, NULL, false);
        C3D_TexSetFilter(outTex, GPU_NEAREST, GPU_NEAREST);
        C3D_TexSetWrap(outTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        fclose(texFile);
        return true;
    }
    else
        return false;
}

uint8_t encodeNormalComponent(float value) {
    if (value == -1.0f) return 0;
    if (value == 0.0f) return 1;
    if (value == 1.0f) return 2;
    return 1;
}

uint8_t compressNormals(float x, float y, float z)
{
    uint8_t ex = encodeNormalComponent(x);
    uint8_t ey = encodeNormalComponent(y);
    uint8_t ez = encodeNormalComponent(z);

    uint8_t result = ((ez & 0b11) << 4) | ((ey & 0b11) << 2) | (ex & 0b11);
    return result;
}

void addCubeFaceToCubicMesh(CubicInstance *instance, CubeFace face, int x, int y, int z)
{
    for (int i = 0; i < 4; i++)
    {
        CompressedVertex local_vtx = {
            .position = {(uint8_t)(face.vertex[i].position[0] + x + 0.5), (uint8_t)(face.vertex[i].position[1] + y + 0.5), (uint8_t)(face.vertex[i].position[2] + z + 0.5)},
            .texcoord = {face.vertex[i].texcoord[0], face.vertex[i].texcoord[1]},
            .normal = compressNormals(face.vertex[i].normal[0], face.vertex[i].normal[1], face.vertex[i].normal[2])
        };
        instance->vbo[instance->vtx_count + i] = local_vtx;
    }

    for (int i = 0; i < 6; i++)
        instance->ibo[instance->idx_count + i] = vindexes[i] + instance->faces_count * 4;
    
    instance->faces_count += 1;
    instance->vtx_count += 4;
    instance->idx_count += 6;
}