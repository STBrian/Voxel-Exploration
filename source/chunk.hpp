#pragma once

#include <citro3d.h>

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "geometry.hpp"

class Chunk
{
    public:
        static const uint8_t CHUNK_SIZE = 16;
        static const uint16_t CHUNK_HEIGHT = 256;
        int CHUNK_X, CHUNK_Z;
        int seed = 2342;
        bool renderReady = false;

    public:
        Chunk(int x, int z) : CHUNK_X(x), CHUNK_Z(z) {}
        ~Chunk() 
        {
            if (this->renderReady)
            {
                uint8_t fragment_n = this->CHUNK_HEIGHT / this->CHUNK_SIZE;
                for (uint8_t fragment = 0; fragment < fragment_n; fragment++)
                {
                    linearFree(this->VBOs[fragment]);
                    linearFree(this->IBOs[fragment]);
                }
            }
        };

        void setChunkBlock(uint8_t x, uint8_t y, uint8_t z, uint8_t blockID);
        uint8_t getChunkBlock(uint8_t x, uint8_t y, uint8_t z);
        void generateChunkData();
        void generateRenderObject();
        void renderChunk();

    private:
        std::unordered_map<uint8_t, std::vector<uint8_t>> layered_blocks_data;
        std::unordered_map<uint8_t, CompressedVertex *> VBOs;
        std::unordered_map<uint8_t, uint16_t *> IBOs;
        std::unordered_map<uint8_t, uint16_t> facesPerFragment;
};