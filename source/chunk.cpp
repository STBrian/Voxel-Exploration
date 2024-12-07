#include "chunk.hpp"

#include <iostream>
#include <string>
#include <algorithm>

#include "FastNoiseLite.h"

#include "render3d.hpp"
#include "render_utils.hpp"

void Chunk::setChunkBlock(uint8_t x, uint8_t y, uint8_t z, uint8_t blockID)
{
    if (blockID == 0)
    {
        auto it = this->layered_blocks_data.find(y);
        if (it != this->layered_blocks_data.end())
        {
            auto& layer = it->second;
            layer[x * this->CHUNK_SIZE + z] = blockID;

            // Verify if the layer is empty
            bool isEmpty = std::all_of(layer.begin(), layer.end(), [](int id) { return id == 0; });
            if (isEmpty)
            {
                this->layered_blocks_data.erase(it);
            }
        }
    }
    else
    {
        // Create the layer if doesn't exists
        if (this->layered_blocks_data.find(y) == this->layered_blocks_data.end())
            this->layered_blocks_data[y] = std::vector<uint8_t>(this->CHUNK_SIZE * this->CHUNK_SIZE, 0);

        this->layered_blocks_data[y][x * this->CHUNK_SIZE + z] = blockID;
    }
}

uint8_t Chunk::getChunkBlock(uint8_t x, uint8_t y, uint8_t z)
{
    auto it = this->layered_blocks_data.find(y);
    if (it != this->layered_blocks_data.end())
    {
        auto& layer = it->second;
        return layer[x * this->CHUNK_SIZE + z];
    }
    else
        return 0;
}

void Chunk::generateChunkData()
{
    FastNoiseLite noise;
    noise.SetSeed(seed);

    int8_t octaves = 4;
    double heightOffset = 0.5;

    noise.SetFractalOctaves(octaves);

    for (uint8_t z = 0; z < this->CHUNK_SIZE; z++)
    {
        for (uint8_t x = 0; x < this->CHUNK_SIZE; x++)
        {
            int ax = x + this->CHUNK_X * this->CHUNK_SIZE;
            int az = z + this->CHUNK_Z * this->CHUNK_SIZE;

            noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
            noise.SetFrequency(0.05f);
            double continentalNoise = noise.GetNoise((float)ax, (float)az);
            double baseDensity = (continentalNoise + 1.0) * 0.5;
            
            noise.SetFrequency(0.04f);
            double erosionNoise = noise.GetNoise((float)ax, (float)az);
            double erosionFactor = 0.1 * (1.0 - erosionNoise);

            noise.SetFrequency(0.1f);
            double peaksAndValleysNoise = noise.GetNoise((float)ax, (float)az);
            double peaksAndValleyFactor = 0.5 + peaksAndValleysNoise * 0.5;

            noise.SetFrequency(0.1f);
            for (uint16_t y = 0; y < this->CHUNK_HEIGHT; y++)
            {
                noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                double caveNoise = noise.GetNoise((float)ax, (float)y, (float)az);
                double caveNoiseFactor = (caveNoise + 1.0) * 0.5;

                double density = baseDensity + peaksAndValleyFactor - erosionFactor * (y * 0.01) - caveNoiseFactor * 0.3;

                if (density > 0.4)
                    this->setChunkBlock(x, y, z, 1);
                else
                    this->setChunkBlock(x, y, z, 0);
            }
        }
    }
}

void Chunk::generateRenderObject()
{
    uint8_t fragment_n = this->CHUNK_HEIGHT / this->CHUNK_SIZE;
    for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
    {
        int n_faces = 0;
        for (uint8_t y = 0; y < this->CHUNK_SIZE; y++)
        {
            for (uint8_t x = 0; x < this->CHUNK_SIZE; x++)
            {
                for (uint8_t z = 0; z < this->CHUNK_SIZE; z++)
                {
                    uint8_t blockID = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z);
                    uint8_t adyacent_block;

                    if (blockID != 0)
                    {
                        if (z < this->CHUNK_SIZE - 1)
                        {
                            adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z + 1);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;

                        if (z > 0)
                        {
                            adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z - 1);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;

                        if (x < this->CHUNK_SIZE - 1)
                        {
                            adyacent_block = this->getChunkBlock(x + 1, y + fragment * this->CHUNK_SIZE, z);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;
                        
                        if (x > 0)
                        {
                            adyacent_block = this->getChunkBlock(x - 1, y + fragment * this->CHUNK_SIZE, z);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;

                        if (y + fragment * this->CHUNK_SIZE < this->CHUNK_HEIGHT - 1)
                        {
                            adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE + 1, z);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;

                        if (y + fragment * this->CHUNK_SIZE > 0)
                        {
                            adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE - 1, z);
                            if (adyacent_block == 0)
                                n_faces++;
                        }
                        else
                            n_faces++;
                    }
                }
            }
        }

        this->facesPerFragment[fragment] = n_faces;
        if (n_faces > 0)
        {
            this->VBOs[fragment] = (CompressedVertex *)linearAlloc(sizeof(CompressedVertex) * n_faces * 4);
            this->IBOs[fragment] = (uint16_t *)linearAlloc(sizeof(vindexes) * n_faces);
            
            int a_faces = 0;
            for (uint8_t y = 0; y < this->CHUNK_SIZE; y++)
            {
                for (uint8_t x = 0; x < this->CHUNK_SIZE; x++)
                {
                    for (uint8_t z = 0; z < this->CHUNK_SIZE; z++)
                    {
                        uint8_t blockID = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z);
                        uint8_t adyacent_block;

                        if (blockID != 0)
                        {
                            // Front
                            if (z < this->CHUNK_SIZE - 1)
                            {
                                adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z + 1);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.front, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.front, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Back
                            if (z > 0)
                            {
                                adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE, z - 1);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.back, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.back, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Right
                            if (x < this->CHUNK_SIZE - 1)
                            {
                                adyacent_block = this->getChunkBlock(x + 1, y + fragment * this->CHUNK_SIZE, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.right, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.right, 
                                    x, y, z
                                );
                                a_faces++;
                            }
                            
                            // Left
                            if (x > 0)
                            {
                                adyacent_block = this->getChunkBlock(x - 1, y + fragment * this->CHUNK_SIZE, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.left, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.left, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Top
                            if (y + fragment * this->CHUNK_SIZE < this->CHUNK_HEIGHT - 1)
                            {
                                adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE + 1, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.top, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.top, 
                                    x, y, z
                                );
                                a_faces++;
                            }

                            // Bottom
                            if (y + fragment * this->CHUNK_SIZE > 0)
                            {
                                adyacent_block = this->getChunkBlock(x, y + fragment * this->CHUNK_SIZE - 1, z);
                                if (adyacent_block == 0)
                                {
                                    addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                        a_faces * 4, a_faces * 6, a_faces, cube.bottom, 
                                        x, y, z
                                    );
                                    a_faces++;
                                }
                            }
                            else
                            {
                                addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                    a_faces * 4, a_faces * 6, a_faces, cube.bottom, 
                                    x, y, z
                                );
                                a_faces++;
                            }
                        }
                    }
                }
            }
        }
    }

    this->renderReady = true;
}

void Chunk::renderChunk()
{
    if (this->renderReady)
    {
        uint8_t fragment_n = this->CHUNK_HEIGHT / this->CHUNK_SIZE;
        for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
        {
            if (this->facesPerFragment[fragment] > 0)
            {
                C3D_Mtx world;
                Mtx_Identity(&world);
                Mtx_Translate(&world, this->CHUNK_SIZE * this->CHUNK_X - 0.5, this->CHUNK_SIZE * fragment - 0.5, this->CHUNK_SIZE * this->CHUNK_Z - 0.5, false);
                Mtx_RotateY(&world, C3D_AngleFromDegrees(0.0f), true);
                Render3D::getInstance().setWorldViewMatrix(&world);

                C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, Render3D::getInstance().getLocModelView(), &world);

                C3D_BufInfo *bufInfo = C3D_GetBufInfo();
                BufInfo_Init(bufInfo);
                BufInfo_Add(bufInfo, this->VBOs[fragment], sizeof(CompressedVertex), 3, 0x210);

                C3D_DrawElements(GPU_TRIANGLES, (this->facesPerFragment[fragment]) * 6, C3D_UNSIGNED_SHORT, this->IBOs[fragment]);
            }
        }
    }
}