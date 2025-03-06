#include "chunk.h"

#include <3ds.h>
#include <citro3d.h>

#include <math.h>

#define FNL_IMPL
#include "FastNoiseLite.h"

#include "render_utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void ChunkInit(Chunk* chunk_s, uint8_t size, uint16_t height) {
    chunk_s->size = size;
    chunk_s->height = height;
    chunk_s->x = 0;
    chunk_s->z = 0;
    chunk_s->render_ready = false;
    chunk_s->layered_blocks_data = calloc(height, sizeof(uint8_t*));
    chunk_s->fragments = calloc(height / size, sizeof(CubicInstance));
    chunk_s->facesPerFragment = calloc(height / size, sizeof(uint16_t));
    chunk_s->front_neighbor = NULL;
    chunk_s->back_neighbor = NULL;
    chunk_s->right_neighbor = NULL;
    chunk_s->left_neighbor = NULL;
}

void ChunkInitDefault(Chunk* chunk_s) {
    ChunkInit(chunk_s, 16, 256);
}

void ChunkSetBlock(Chunk* chunk_s, uint8_t x, uint8_t y, uint8_t z, uint8_t blockID) {
    if (blockID == 0)
    {
        uint8_t* layer = chunk_s->layered_blocks_data[y];
        if (layer != NULL)
        {
            layer[x * chunk_s->size + z] = blockID;

            // Free layer if is empty
            bool isEmpty = true;
            int size = chunk_s->size * chunk_s->size;
            int i = 0;
            while (isEmpty && i < size)
            {
                if (layer[i] != 0)
                {
                    isEmpty = false;
                    break;
                }
                i++;
            }

            if (isEmpty)
            {
                free(layer);
                chunk_s->layered_blocks_data[y] = NULL;
            }
        }
    }
    else
    {
        // Create layer if doesn't exists
        if (chunk_s->layered_blocks_data[y] == NULL)
            chunk_s->layered_blocks_data[y] = calloc(chunk_s->size * chunk_s->size, sizeof(uint8_t));

        uint8_t* layer = chunk_s->layered_blocks_data[y];
        layer[x * chunk_s->size + z] = blockID;
    }
}

uint8_t ChunkGetBlock(Chunk* chunk_s, int x, int y, int z) {
    if ((x < 0 || x >= chunk_s->size) && (z < 0 || z >= chunk_s->size))
        return 0;
    if (x < 0)
    {
        if (chunk_s->left_neighbor != NULL)
            return ChunkGetBlock(chunk_s->left_neighbor, chunk_s->size + x, y, z);
        else
            return 1;
    }
    else if (x >= chunk_s->size)
    {
        if (chunk_s->right_neighbor != NULL)
            return ChunkGetBlock(chunk_s->right_neighbor, x - chunk_s->size, y, z);
        else
            return 1;
    }
    if (z < 0)
    {
        if (chunk_s->back_neighbor != NULL)
            return ChunkGetBlock(chunk_s->back_neighbor, x, y, chunk_s->size + z);
        else
            return 1;
    }
    else if (z >= chunk_s->size)
    {
        if (chunk_s->front_neighbor != NULL)
            return ChunkGetBlock(chunk_s->front_neighbor, x, y, z - chunk_s->size);
        else
            return 1;
    }
    uint8_t* layer = chunk_s->layered_blocks_data[y];
    if (layer != NULL)
        return layer[x * chunk_s->size + z];
    else
        return 0;
}

void ChunkGenerateTerrain(Chunk* chunk_s, int seed) {
    fnl_state noise = fnlCreateState();
    noise.seed = seed;
    noise.octaves = 4;

    for (uint8_t z = 0; z < chunk_s->size; z++)
    {
        for (uint8_t x = 0; x < chunk_s->size; x++)
        {
            int ax = x + chunk_s->x * chunk_s->size;
            int az = z + chunk_s->z * chunk_s->size;

            double baseHeight = 70;

            noise.noise_type = FNL_NOISE_PERLIN;
            noise.frequency = 0.005f;
            double continentalNoise = fnlGetNoise2D(&noise, (float)ax, (float)az);
            double finalHeight = baseHeight + (continentalNoise * 20);

            noise.frequency = 0.01f;
            double erosionNoise = fnlGetNoise2D(&noise, (float)ax, (float)az);
            finalHeight -= ((erosionNoise + 1) * 4);

            noise.frequency = 0.05f;
            double peaksAndValleysNoise = fnlGetNoise2D(&noise, (float)ax, (float)az);
            finalHeight += (peaksAndValleysNoise * 4);

            finalHeight = MIN(chunk_s->height, finalHeight);

            noise.frequency = 0.02f;
            noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
            for (uint16_t y = 0; y < finalHeight; y++)
            {
                double caveNoise = fnlGetNoise3D(&noise, (float)ax, (float)y, (float)az);
                double heightFactor = 0.115 * (finalHeight / chunk_s->height * y);
                double density = caveNoise + heightFactor;

                if (density > -0.2 && density < 0.2)
                    ChunkSetBlock(chunk_s, x, y, z, 0);
                else if (density < -0.9)
                    ChunkSetBlock(chunk_s, x, y, z, 0);
                else if (density > 0.9 && density <= 1.0)
                    ChunkSetBlock(chunk_s, x, y, z, 0);
                else
                    ChunkSetBlock(chunk_s, x, y, z, 1);
            }
        }
    }
}

void ChunkGenerateRenderObject(Chunk* chunk_s) {
    uint8_t fragment_n = chunk_s->height / chunk_s->size;
    for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
    {
        uint16_t *z_left = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        uint16_t *z_right = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        uint16_t *x_left = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        uint16_t *x_right = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        uint16_t *y_left = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        uint16_t *y_right = (uint16_t*)malloc(sizeof(uint16_t) * 16 * 16);
        // Calculate faces
        int n_faces = 0;
        for (uint8_t y = 0; y < chunk_s->size; y++)
        {
            if (chunk_s->layered_blocks_data[y + fragment * chunk_s->size] != NULL)
            {
                for (uint8_t x = 0; x < chunk_s->size; x++)
                {
                    uint16_t z_row = 0;
                    for (uint8_t z = 0; z < chunk_s->size; z++)
                    {
                        uint8_t blockID = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z);
                        if (blockID)
                            z_row |= (1 << z);
                    }
                    if (ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, -1))
                        z_left[y * 16 + x] = (z_row << 1) | 1;
                    else
                        z_left[y * 16 + x] = z_row << 1;
                    z_left[y * 16 + x] = ~z_left[y * 16 + x];
                    if (ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, chunk_s->size))
                        z_right[y * 16 + x] = (z_row >> 1) | (1 << 15);
                    else
                        z_right[y * 16 + x] = z_row >> 1;
                    z_right[y * 16 + x] = ~z_right[y * 16 + x];

                    z_left[y * 16 + x] = z_row & z_left[y * 16 + x]; // back
                    z_right[y * 16 + x] = z_row & z_right[y * 16 + x]; // front

                    for (uint8_t z = 0; z < chunk_s->size; z++)
                    {
                        if ((z_left[y * 16 + x] >> z) & 0b1)
                            n_faces++;
                        if ((z_right[y * 16 + x] >> z) & 0b1)
                            n_faces++;
                    }
                }

                for (uint8_t z = 0; z < chunk_s->size; z++)
                {
                    uint16_t x_row = 0;
                    for (uint8_t x = 0; x < chunk_s->size; x++)
                    {
                        uint8_t blockID = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z);
                        if (blockID != 0)
                            x_row |= (1 << x);
                    }
                    if (ChunkGetBlock(chunk_s, -1, y + fragment * chunk_s->size, z))
                        x_left[y * 16 + z] = (x_row << 1) | 1;
                    else
                        x_left[y * 16 + z] = x_row << 1;
                    x_left[y * 16 + z] = ~x_left[y * 16 + z];
                    if (ChunkGetBlock(chunk_s, chunk_s->size, y + fragment * chunk_s->size, z))
                        x_right[y * 16 + z] = (x_row >> 1) | (1 << 15);
                    else
                        x_right[y * 16 + z] = x_row >> 1;
                    x_right[y * 16 + z] = ~x_right[y * 16 + z];

                    x_left[y * 16 + z] = x_row & x_left[y * 16 + z]; // left
                    x_right[y * 16 + z] = x_row & x_right[y * 16 + z]; // right
                    
                    for (uint8_t x = 0; x < chunk_s->size; x++)
                    {
                        if ((x_left[y * 16 + z] >> x) & 0b1)
                            n_faces++;
                        if ((x_right[y * 16 + z] >> x) & 0b1)
                            n_faces++;
                    }
                }
            }
        }

        for (uint8_t x = 0; x < chunk_s->size; x++)
        {
            for (uint8_t z = 0; z < chunk_s->size; z++)
            {
                uint16_t y_row = 0;
                for (uint8_t y = 0; y < chunk_s->size; y++)
                {
                    if (chunk_s->layered_blocks_data[y + fragment * chunk_s->size] != NULL)
                    {
                        uint8_t blockID = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z);
                        if (blockID)
                            y_row |= (1 << y);
                    }
                }
                if (fragment > 0)
                {
                    if (ChunkGetBlock(chunk_s, x, -1 + fragment * chunk_s->size, z))
                        y_left[x * 16 + z] = (y_row << 1) | 1;
                    else
                        y_left[x * 16 + z] = y_row << 1;
                }
                else
                    y_left[x * 16 + z] = y_row << 1;
                y_left[x * 16 + z] = ~y_left[x * 16 + z];
                if (fragment < fragment_n - 1)
                {
                    if (ChunkGetBlock(chunk_s, x, chunk_s->size + fragment * chunk_s->size, z))
                        y_right[x * 16 + z] = (y_row >> 1) | (1 << 15);
                    else
                        y_right[x * 16 + z] = y_row >> 1;
                }
                else
                    y_right[x * 16 + z] = y_row >> 1;
                y_right[x * 16 + z] = ~y_right[x * 16 + z];

                y_left[x * 16 + z] = y_row & y_left[x * 16 + z]; // bottom
                y_right[x * 16 + z] = y_row & y_right[x * 16 + z]; // top

                for (uint8_t y = 0; y < chunk_s->size; y++)
                {
                    if ((y_left[x * 16 + z] >> y) & 0b1)
                        n_faces++;
                    if ((y_right[x * 16 + z] >> y) & 0b1)
                        n_faces++;
                }
            }
        }

        // Adds the calculated faces to vbo and ibo
        chunk_s->facesPerFragment[fragment] = n_faces;
        if (n_faces > 0)
        {
            chunk_s->fragments[fragment].vbo = (CompressedVertex*)linearAlloc(sizeof(CompressedVertex) * n_faces * 4);
            chunk_s->fragments[fragment].ibo = (uint16_t*)linearAlloc(sizeof(vindexes) * n_faces);

            for (uint8_t y = 0; y < chunk_s->size; y++)
            {
                if (chunk_s->layered_blocks_data[y + fragment * chunk_s->size] != NULL)
                {
                    for (uint8_t x = 0; x < chunk_s->size; x++)
                    {
                        for (uint8_t z = 0; z < chunk_s->size; z++)
                        {
                            if ((z_left[y * 16 + x] >> z) & 0b1) // back
                                addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.back, x, y, z);
                            if ((z_right[y * 16 + x] >> z) & 0b1) // front
                                addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.front, x, y, z);
                        }
                    }

                    for (uint8_t z = 0; z < chunk_s->size; z++)
                    {
                        for (uint8_t x = 0; x < chunk_s->size; x++)
                        {
                            if ((x_left[y * 16 + z] >> x) & 0b1) // left
                                addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.left, x, y, z);
                            if ((x_right[y * 16 + z] >> x) & 0b1) // right
                                addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.right, x, y, z);
                        }
                    }
                }
            }

            for (uint8_t x = 0; x < chunk_s->size; x++)
            {
                for (uint8_t z = 0; z < chunk_s->size; z++)
                {
                    for (uint8_t y = 0; y < chunk_s->size; y++)
                    {
                        if ((y_left[x * 16 + z] >> y) & 0b1) // bottom
                            addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.bottom, x, y, z);
                        if ((y_right[x * 16 + z] >> y) & 0b1) // top
                            addCubeFaceToCubicMesh(&chunk_s->fragments[fragment], cube.top, x, y, z);
                    }
                }
            }
        }
        free(z_left);
        free(z_right);
        free(x_left);
        free(x_right);
        free(y_left);
        free(y_right);
    }
    chunk_s->render_ready = true;
}

void ChunkRender(Chunk* chunk_s, RCamera* actorCamera)
{
    if (chunk_s->render_ready)
    {
        uint8_t fragment_n = chunk_s->height / chunk_s->size;
        for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
        {
            double fragDistance = fabs(fragment * chunk_s->size - actorCamera->position.y + chunk_s->size / 2);
            if (fragDistance < chunk_s->size * 2) {
                if (chunk_s->facesPerFragment[fragment] > 0)
                {
                    C3D_Mtx world;
                    Mtx_Identity(&world);
                    Mtx_Translate(&world, chunk_s->size * chunk_s->x - 0.5, chunk_s->size * fragment - 0.5, chunk_s->size * chunk_s->z - 0.5, false);
                    Mtx_RotateY(&world, C3D_AngleFromDegrees(0.0f), true);

                    R3D_SetModelViewMatrx(&world);
                    R3D_DrawCubicInstance(&chunk_s->fragments[fragment]);
                }
            }
        }
    }
}

void ChunkFreeRenderObject(Chunk* chunk_s)
{
    uint8_t fragment_n = chunk_s->height / chunk_s->size;
    for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
    {
        if (chunk_s->fragments[fragment].vbo != NULL)
            linearFree(chunk_s->fragments[fragment].vbo);
        if (chunk_s->fragments[fragment].ibo != NULL)
            linearFree(chunk_s->fragments[fragment].ibo);
    }
    chunk_s->render_ready = false;
}

void ChunkDestroy(Chunk* chunk_s)
{
    ChunkFreeRenderObject(chunk_s);
    free(chunk_s->fragments);
    for (int i = 0; i < chunk_s->height; i++)
        if (chunk_s->layered_blocks_data[i] != NULL)
            free(chunk_s->layered_blocks_data[i]);
    free(chunk_s->layered_blocks_data);
    free(chunk_s->facesPerFragment);
}