#include "render_utils.h"

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

void addCubeFaceToMesh(CompressedVertex *vbo, uint16_t *ibo, uint32_t vbo_idx, uint32_t ibo_idx, uint16_t face_idx, CubeFace face, int x, int y, int z)
{
    for (int i = 0; i < 4; i++)
    {
        CompressedVertex local_vtx = {
            .position = {(uint8_t)(face.vertex[i].position[0] + x + 0.5), (uint8_t)(face.vertex[i].position[1] + y + 0.5), (uint8_t)(face.vertex[i].position[2] + z + 0.5)},
            .texcoord = {face.vertex[i].texcoord[0], face.vertex[i].texcoord[1]},
            .normal = compressNormals(face.vertex[i].normal[0], face.vertex[i].normal[1], face.vertex[i].normal[2])
        };
        vbo[vbo_idx + i] = local_vtx;
    }

    for (int i = 0; i < 6; i++)
        ibo[ibo_idx + i] = vindexes[i] + face_idx * 4;
}