#include "chunk.h"

#include <3ds.h>
#include <citro3d.h>

#define FNL_IMPL
#include "FastNoiseLite.h"

#include "render3d.h"
#include "render_utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void ChunkInit(Chunk* chunk_s, uint8_t size, uint16_t height) {
    chunk_s->size = size;
    chunk_s->height = height;
    chunk_s->x = 0;
    chunk_s->z = 0;
    chunk_s->render_ready = false;
    chunk_s->layered_blocks_data = calloc(height, sizeof(uint8_t*));
    chunk_s->VBOs = calloc(height / size, sizeof(CompressedVertex*));
    chunk_s->IBOs = calloc(height / size, sizeof(uint16_t*));
    chunk_s->facesPerFragment = calloc(height / size, sizeof(uint16_t));
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
                    isEmpty = false;
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

uint8_t ChunkGetBlock(Chunk* chunk_s, uint8_t x, uint8_t y, uint8_t z) {
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
        int n_faces = 0;
        for (uint8_t y = 0; y < chunk_s->size; y++)
        {
            if (chunk_s->layered_blocks_data[y + fragment * chunk_s->size] != NULL)
            {
                for (uint8_t x = 0; x < chunk_s->size; x++)
                {
                    int row_z = 0;
                    for (uint8_t z = 0; z < chunk_s->size; z++)
                    {
                        uint8_t blockID = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z);
                        uint8_t adyacent_block;

                        if (blockID != 0)
                        {
                            if (z < chunk_s->size - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z + 1);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;

                            if (z > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z - 1);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;

                            if (x < chunk_s->size - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x + 1, y + fragment * chunk_s->size, z);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;
                            
                            if (x > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x - 1, y + fragment * chunk_s->size, z);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;

                            if (y + fragment * chunk_s->size < chunk_s->height - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size + 1, z);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;

                            if (y + fragment * chunk_s->size > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size - 1, z);
                                if (adyacent_block == 0)
                                    n_faces++;
                            }
                            else
                                n_faces++;
                        }
                    }
                }
            }
        }

        chunk_s->facesPerFragment[fragment] = n_faces;
        if (n_faces > 0)
        {
            chunk_s->VBOs[fragment] = (CompressedVertex*)linearAlloc(sizeof(CompressedVertex) * n_faces * 4);
            chunk_s->IBOs[fragment] = (uint16_t*)linearAlloc(sizeof(vindexes) * n_faces);

            int a_faces = 0;
            for (uint8_t y = 0; y < chunk_s->size; y++)
            {
                for (uint8_t x = 0; x < chunk_s->size; x++)
                {
                    for (uint8_t z = 0; z < chunk_s->size; z++)
                    {
                        uint8_t blockID = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z);
                        uint8_t adyacent_block;

                        if (blockID != 0)
                        {
                            // Front
                            if (z < chunk_s->size - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z + 1);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.front, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.front, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Back
                            if (z > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size, z - 1);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.back, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.back, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Right
                            if (x < chunk_s->size - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x + 1, y + fragment * chunk_s->size, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.right, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.right, 
                                    x, y, z
                                );
                                a_faces++;
                            }
                            
                            // Left
                            if (x > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x - 1, y + fragment * chunk_s->size, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.left, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.left, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Top
                            if (y + fragment * chunk_s->size < chunk_s->height - 1)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size + 1, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.top, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.top, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Bottom
                            if (y + fragment * chunk_s->size > 0)
                            {
                                adyacent_block = ChunkGetBlock(chunk_s, x, y + fragment * chunk_s->size - 1, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.bottom, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.bottom, 
                                    x, y, z
                                );
                                a_faces++;
                            }
                        }
                    }
                }
            }

            chunk_s->render_ready = true;
        }
    }
}

void ChunkRender(Chunk* chunk_s)
{
    if (chunk_s->render_ready)
    {
        uint8_t fragment_n = chunk_s->height / chunk_s->size;
        for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
        {
            if (chunk_s->facesPerFragment[fragment] > 0)
            {
                C3D_Mtx world;
                Mtx_Identity(&world);
                Mtx_Translate(&world, chunk_s->size * chunk_s->x - 0.5, chunk_s->size * fragment - 0.5, chunk_s->size * chunk_s->z - 0.5, false);
                Mtx_RotateY(&world, C3D_AngleFromDegrees(0.0f), true);

                // TODO: Rewrite 3D render to C
                gsetModelViewMatrx(&world);
                grenderOptimizedInstance(chunk_s->VBOs[fragment], chunk_s->IBOs[fragment], chunk_s->facesPerFragment[fragment] * 6);
            }
        }
    }
}

void ChunkFreeRenderObject(Chunk* chunk_s)
{
    uint8_t fragment_n = chunk_s->height / chunk_s->size;
    for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
    {
        if (chunk_s->VBOs[fragment] != NULL)
            linearFree(chunk_s->VBOs[fragment]);
        if (chunk_s->IBOs[fragment] != NULL)
            linearFree(chunk_s->IBOs[fragment]);
    }
    chunk_s->render_ready = false;
}

void ChunkDestroy(Chunk* chunk_s)
{
    ChunkFreeRenderObject(chunk_s);
    for (int i = 0; i < chunk_s->height; i++)
        if (chunk_s->layered_blocks_data[i] != NULL)
            free(chunk_s->layered_blocks_data[i]);
    free(chunk_s->layered_blocks_data);
    free(chunk_s->facesPerFragment);
}