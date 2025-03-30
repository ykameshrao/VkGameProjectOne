// TerrainLoader.cpp

#include "TerrainLoader.h"
#include "Terrain.h"

#include <spdlog/spdlog.h>
#include <cmath>

// Ensure stb_image is implemented in exactly ONE .cpp file in your project
// If it's implemented elsewhere, just include the header normally.
// If this IS the designated file, uncomment the define:
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // Include stb_image header

namespace VkProjectOne::TerrainLoader {
    // Helper function to sample height (robustly handles boundaries)
    // Assumes 8-bit grayscale or uses first channel of multichannel images
    float getHeight(int x, int z, int width, int height, const stbi_uc *heightmapData, int channels) {
        // Clamp coordinates to valid range
        x = std::max(0, std::min(x, width - 1));
        z = std::max(0, std::min(z, height - 1));

        // Calculate index (handles different channel counts)
        int index = (z * width + x) * channels;
        unsigned char pixelValue = heightmapData[index];

        // Normalize height to [0.0, 1.0]
        return static_cast<float>(pixelValue) / 255.0f;
    }

    // Helper function to calculate normal using finite difference / cross product
    glm::vec3 calculateNormal(int x, int z, int width, int height, float scaleXY, float scaleY,
                              const stbi_uc *heightmapData, int channels) {
        // Get heights of neighboring pixels (using helper to handle boundaries)
        float hl = getHeight(x - 1, z, width, height, heightmapData, channels) * scaleY; // Left
        float hr = getHeight(x + 1, z, width, height, heightmapData, channels) * scaleY; // Right
        float hd = getHeight(x, z - 1, width, height, heightmapData, channels) * scaleY; // Down (back)
        float hu = getHeight(x, z + 1, width, height, heightmapData, channels) * scaleY; // Up (forward)

        // Calculate normal using cross product of vectors along X and Z axes
        // Vector along Z: (0, hu - hd, 2 * scaleXY) assuming Z increases "up" on map
        // Vector along X: (2 * scaleXY, hr - hl, 0) assuming X increases "right" on map
        glm::vec3 normal = glm::normalize(glm::vec3(
            scaleXY * (hl - hr), // X component depends on height difference left-right
            2.0f * scaleXY, // Y component (adjust scale based on XY scale) - this is simplified, see notes
            scaleXY * (hd - hu) // Z component depends on height difference down-up
        ));

        // A more standard cross-product approach:
        // glm::vec3 tangentX = glm::vec3(2.0f * scaleXY, hr - hl, 0.0f);
        // glm::vec3 tangentZ = glm::vec3(0.0f, hu - hd, 2.0f * scaleXY);
        // normal = glm::normalize(glm::cross(tangentZ, tangentX)); // Order matters for direction

        return normal;
    }

    bool LoadFromHeightmap(
        const std::string &heightmapPath,
        float scaleXY,
        float scaleY,
        std::vector<TerrainVertex> &outVertices,
        std::vector<uint32_t> &outIndices) {
        spdlog::info("Loading terrain from heightmap: {}", heightmapPath);

        int width, height, channels;
        // Load image data using stb_image
        // stbi_load returns unsigned char* = array of pixel data
        stbi_uc *pixels = stbi_load(heightmapPath.c_str(), &width, &height, &channels, 0);

        if (!pixels) {
            spdlog::error("Failed to load heightmap image: {}", heightmapPath);
            // You might want to check stbi_failure_reason() here
            return false;
        }

        spdlog::debug("Heightmap loaded: {}x{} pixels, {} channels", width, height, channels);
        if (channels != 1 && channels != 3 && channels != 4) {
            spdlog::warn("Heightmap has {} channels. Using first channel only.", channels);
            // Note: You might want to enforce grayscale or handle RGB differently
        }


        outVertices.clear();
        outIndices.clear();
        outVertices.reserve(static_cast<size_t>(width) * height); // Pre-allocate memory
        outIndices.reserve(static_cast<size_t>(width - 1) * (height - 1) * 6); // 6 indices per quad

        // Generate Vertices
        spdlog::debug("Generating terrain vertices...");
        for (int z = 0; z < height; ++z) {
            for (int x = 0; x < width; ++x) {
                TerrainVertex vertex;

                // Position
                float terrainHeight = getHeight(x, z, width, height, pixels, channels) * scaleY;
                vertex.pos = glm::vec3(x * scaleXY, terrainHeight, z * scaleXY);

                // Normal (Calculate based on neighbors)
                vertex.normal = calculateNormal(x, z, width, height, scaleXY, scaleY, pixels, channels);

                // Texture Coordinates (Stretch texture over the whole terrain)
                vertex.texCoord = glm::vec2(
                    static_cast<float>(x) / static_cast<float>(width - 1),
                    static_cast<float>(z) / static_cast<float>(height - 1)
                );

                outVertices.push_back(vertex);
            }
        }
        spdlog::debug("Generated {} vertices.", outVertices.size());


        // Generate Indices (Triangle List for grid)
        spdlog::debug("Generating terrain indices...");
        for (int z = 0; z < height - 1; ++z) {
            for (int x = 0; x < width - 1; ++x) {
                // Indices for the 4 corners of the quad
                uint32_t topLeft = z * width + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * width + x;
                uint32_t bottomRight = bottomLeft + 1;

                // Add indices for the two triangles forming the quad
                // Triangle 1: Top-Left -> Bottom-Left -> Top-Right
                outIndices.push_back(topLeft);
                outIndices.push_back(bottomLeft);
                outIndices.push_back(topRight);

                // Triangle 2: Top-Right -> Bottom-Left -> Bottom-Right
                outIndices.push_back(topRight);
                outIndices.push_back(bottomLeft);
                outIndices.push_back(bottomRight);
            }
        }
        spdlog::debug("Generated {} indices.", outIndices.size());

        // Free the loaded image data
        stbi_image_free(pixels);
        spdlog::debug("Heightmap image data freed.");

        spdlog::info("Terrain mesh generated successfully ({} vertices, {} indices).", outVertices.size(),
                     outIndices.size());
        return true;
    }
} // namespace VkProjectOne::TerrainLoader
