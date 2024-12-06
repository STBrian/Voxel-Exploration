#include "chunk.hpp"
#include <iostream>
#include <string>

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
    for (uint16_t y = 0; y < this->CHUNK_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < this->CHUNK_SIZE; x++)
        {
            for (uint8_t z = 0; z < this->CHUNK_SIZE; z++)
            {
                uint8_t block = getBlock(this->seed, x + this->CHUNK_X * this->CHUNK_SIZE, y, z + this->CHUNK_Z * this->CHUNK_SIZE);
                this->setChunkBlock(x, y, z, block);
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

        this->VBOs[fragment] = (Vertex *)linearAlloc(sizeof(Vertex) * n_faces * 4);
        this->IBOs[fragment] = (uint16_t *)linearAlloc(sizeof(vindexes) * n_faces);
        this->facesPerFragment[fragment] = n_faces;
        
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.front, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.back, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.right, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.left, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.top, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
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
                                    this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                                );
                                a_faces++;
                            }
                        }
                        else
                        {
                            addCubeFaceToMesh(this->VBOs[fragment], this->IBOs[fragment], 
                                a_faces * 4, a_faces * 6, a_faces, cube.bottom, 
                                this->CHUNK_X * this->CHUNK_SIZE + x, fragment * this->CHUNK_SIZE + y, this->CHUNK_Z * this->CHUNK_SIZE + z
                            );
                            a_faces++;
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
                C3D_BufInfo *bufInfo = C3D_GetBufInfo();
                BufInfo_Init(bufInfo);
                BufInfo_Add(bufInfo, this->VBOs[fragment], sizeof(Vertex), 3, 0x210);

                C3D_DrawElements(GPU_TRIANGLES, (this->facesPerFragment[fragment]) * 6, C3D_UNSIGNED_SHORT, this->IBOs[fragment]);
            }
        }
    }
}