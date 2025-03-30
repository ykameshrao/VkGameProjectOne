
// Terrain.h

#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace VkProjectOne {
    // Structure for a single vertex in the terrain mesh
    struct TerrainVertex {
        glm::vec3 pos;      // Position (X, Y computed from heightmap, Z)
        glm::vec3 normal;   // Normal vector for lighting
        glm::vec2 texCoord; // UV coordinates for texturing

        // Describes the vertex data binding (how data is spaced in the buffer)
        static VkVertexInputBindingDescription getBindingDescription();
        // Describes the individual attributes (pos, normal, texCoord) within a vertex
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const TerrainVertex& other) const {
            return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
        }
    };

    // Class to hold the generated terrain mesh data
    class Terrain {
    public:
        // Constructor (can be simple or take data directly)
        Terrain() = default; // Default constructor
        Terrain(std::vector<TerrainVertex>&& vertices, std::vector<uint32_t>&& indices);

        // --- Prevent Copying (moving might be okay if needed) ---
        Terrain(const Terrain&) = delete;
        Terrain& operator=(const Terrain&) = delete;
        // Allow moving
        Terrain(Terrain&&) = default;
        Terrain& operator=(Terrain&&) = default;

        // --- Accessors ---
        const std::vector<TerrainVertex>& getVertices() const { return vertices; }
        const std::vector<uint32_t>& getIndices() const { return indices; }
        size_t getVertexCount() const { return vertices.size(); }
        size_t getIndexCount() const { return indices.size(); }

        // Friend declaration allows TerrainLoader to directly populate private members
        // Alternatively, add public methods like setVertices/setIndices if preferred.
        // friend class TerrainLoader; // Or make members public/add setters

    // Consider making these private if only Loader should modify them
    private:
        std::vector<TerrainVertex> vertices;
        std::vector<uint32_t> indices; // Use 32-bit indices for potentially large meshes
    };

} // namespace VkProjectOne