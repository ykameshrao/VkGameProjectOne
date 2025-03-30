
// VulkanEngine.h

#pragma once // Include guard
#define VK_ENABLE_BETA_EXTENSIONS 1
// External dependencies used in the header
#include <vector>
#include <optional>
#include <string> // For exception messages
#include <stdexcept> // For std::runtime_error base class
#include <vulkan/vulkan.h>

// GLM math library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan clip space is [0, 1]
#include <glm/glm.hpp>

// Forward declare SDL_Window instead of including full SDL.h
struct SDL_Window;

namespace VkGameProjectOne {
    // Simple vertex structure matching shader layout(location=...)
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;

        // Describes how to load vertex data (stride, input rate)
        static VkVertexInputBindingDescription getBindingDescription();

        // Describes individual vertex attributes (position, color)
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    // Uniform Buffer Object structure matching shader layout(binding=0)
    // Use alignas to ensure proper alignment for mat4 according to Vulkan spec
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // Structure to hold queue family indices
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanEngine {
    public:
        // Custom exception class for handling swapchain invalidation during rendering
        class SwapChainOutOfDateError : public std::runtime_error {
        public:
            SwapChainOutOfDateError() : std::runtime_error("Swap chain out of date/suboptimal and needs recreation") {
            }
        };

        // Constructor initializes Vulkan with the given SDL window. Throws on failure.
        explicit VulkanEngine(SDL_Window *sdlWindow);

        // Destructor cleans up all Vulkan resources.
        ~VulkanEngine();

        // --- Prevent Copying/Moving ---
        VulkanEngine(const VulkanEngine &) = delete;

        VulkanEngine &operator=(const VulkanEngine &) = delete;

        VulkanEngine(VulkanEngine &&) = delete;

        VulkanEngine &operator=(VulkanEngine &&) = delete;

        // Main rendering function to draw a single frame.
        // Throws SwapChainOutOfDateError if swapchain needs recreation.
        void drawFrame();

        // Recreates the swapchain and dependent resources (e.g., after window resize).
        void recreateSwapChain();

        // Call this when the window framebuffer is resized (e.g., from SDL resize event).
        void notifyFramebufferResized() { framebufferResized = true; }

        // Updates the model matrix in the uniform buffer based on time for rotation.
        void updateCubeRotation(float time);

    private:
        // --- Core Objects ---
        SDL_Window *window = nullptr; // Non-owning pointer to the SDL window
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE; // Logical device
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        // --- Swapchain ---
        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D swapChainExtent = {0, 0};
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        // --- Pipeline ---
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // For UBO
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;

        // --- Commands ---
        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers; // One per frame in flight

        // --- Buffers ---
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void *> uniformBuffersMapped; // Persistently mapped pointers

        // --- Descriptors ---
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        // --- Synchronization ---
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        uint32_t currentFrame = 0;
        bool framebufferResized = false; // Flag to signal swapchain recreation needed


        // --- Private Initialization Methods ---
        void initVulkan();

        void createInstance();

        void setupDebugMessenger();

        void createSurface();

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createSwapChain();

        void createImageViews();

        void createRenderPass();

        void createDescriptorSetLayout();

        void createGraphicsPipeline();

        void createFramebuffers();

        void createCommandPool();

        void createVertexBuffer();

        void createIndexBuffer();

        void createUniformBuffers();

        void createDescriptorPool();

        void createDescriptorSets();

        void createCommandBuffers();

        void createSyncObjects();

        // --- Private Rendering & Update Methods ---
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        // Updates the uniform buffer memory for the given frame index with current matrix data.
        void updateUniformBuffer(uint32_t currentImageIndex);

        // --- Private Cleanup Methods ---
        void cleanupSwapChain();

        void cleanup(); // Main cleanup

        // --- Private Helper Methods & Structs ---
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        VkGameProjectOne::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice queryDevice) const;

        static bool checkDeviceExtensionSupport(VkPhysicalDevice queryDevice);

        bool isDeviceSuitable(VkPhysicalDevice queryDevice);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice queryDevice) const;

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        VkCommandBuffer beginSingleTimeCommands();

        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        VkShaderModule createShaderModule(const std::vector<char> &code);

        bool checkValidationLayerSupport(); // Moved check here

        // --- Debug Messenger ---
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        // --- Placeholder Text Rendering Methods ---
        void initTextRendering();

        void drawText(VkCommandBuffer commandBuffer);

        void cleanupTextRendering();
    };
} // namespace VkGameProjectOne
