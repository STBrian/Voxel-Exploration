#include "terrain_generation.hpp"

uint8_t getBlock(int seed, int x, int y, int z)
{
    FastNoiseLite noise;
    noise.SetSeed(seed);

    int8_t octaves = 2;
    
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalOctaves(octaves);
    noise.SetFrequency(0.04f);
    double continentalNoise = noise.GetNoise((float)x, (float)z);
    double baseDensity = (continentalNoise + 1.0) * 0.5;
    
    noise.SetFrequency(0.05f);
    double erosionNoise = noise.GetNoise((float)x, (float)z);
    double erosionFactor = 1.0 - (erosionNoise * 0.2);

    noise.SetFrequency(0.1f);
    double peaksAndValleysNoise = noise.GetNoise((float)x, (float)z);
    double peaksAndValleyFactor = 0.8 * peaksAndValleysNoise;

    noise.SetFrequency(0.035f);
    double caveNoise = noise.GetNoise((float)x, (float)y, (float)z);
    double caveNoiseFactor = (caveNoise + 1.0) * 0.5;

    double density = baseDensity * erosionFactor * peaksAndValleyFactor - (caveNoiseFactor * 0.6);

    if (density > 0)
        return 1;
    else
        return 0;
}