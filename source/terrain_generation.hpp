#pragma once

#include <cstdint>

#include "PerlinNoise.hpp"
#include "FastNoiseLite.h"

uint8_t getBlock(int seed, int x, int y, int z);