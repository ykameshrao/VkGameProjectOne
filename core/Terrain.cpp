
// Terrain.cpp

#include "Terrain.h"
#include <stdexcept>

namespace VkProjectOne {

// --- TerrainVertex Method Implementations ---

VkVertexInputBindingDescription TerrainVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0; // Vertex buffer binding index (usually 0)
    bindingDescription.stride = sizeof(TerrainVertex); // Size of one vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Process one vertex at a time
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> TerrainVertex::getAttributeDescriptions() {
    // We have 3 attributes: Position, Normal, TexCoord
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    // Position Attribute (location = 0 in shader)
    attributeDescriptions[0].binding = 0; // Matches the binding index above
    attributeDescriptions[0].location = 0; // layout(location = 0) in vertex shader
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 (3x 32-bit float)
    attributeDescriptions[0].offset = offsetof(TerrainVertex, pos); // Offset within the struct

    // Normal Attribute (location = 1 in shader)
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1; // layout(location = 1) in vertex shader
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[1].offset = offsetof(TerrainVertex, normal);

    // Texture Coordinate Attribute (location = 2 in shader)
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2; // layout(location = 2) in vertex shader
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2 (2x 32-bit float)
    attributeDescriptions[2].offset = offsetof(TerrainVertex, texCoord);

    return attributeDescriptions;
}

// --- Terrain Method Implementations ---

Terrain::Terrain(std::vector<TerrainVertex>&& vertices, std::vector<uint32_t>&& indices)
    : vertices(std::move(vertices)), indices(std::move(indices))
{}

// Getters are defined inline in the header for simplicity

} // namespace VkProjectOne