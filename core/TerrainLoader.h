// TerrainLoader.h

#pragma once

#include <vector>
#include <string>
#include "Terrain.h"

namespace VkProjectOne::TerrainLoader {
    /**
         * @brief Generates terrain vertex and index data from a grayscale heightmap image.
         * @param heightmapPath Path to the heightmap image file.
         * @param scaleXY Scaling factor for the X and Z dimensions of the terrain grid.
         * @param scaleY Scaling factor for the height (Y dimension) based on pixel intensity.
         * @param outVertices [Output] Vector to be filled with generated TerrainVertex data.
         * @param outIndices [Output] Vector to be filled with generated index data (triangle list).
         * @return True if loading and generation were successful, false otherwise.
         */
    bool LoadFromHeightmap(
        const std::string &heightmapPath,
        float scaleXY,
        float scaleY,
        std::vector<TerrainVertex> &outVertices,
        std::vector<uint32_t> &outIndices);

    // Helper function (optional, could be private in .cpp)
    // float getHeight(int x, int z, int width, int height, const unsigned char* heightmapData, int channels);
    // glm::vec3 calculateNormal(int x, int z, int width, int height, float scaleXY, float scaleY, const unsigned char* heightmapData, int channels);
}
