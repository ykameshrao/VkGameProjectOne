
// VulkanEngine.cpp

#include "VulkanEngine.h"

// Core Libraries
#include <cmake-build-debug/_deps/spdlog-src/include/spdlog/spdlog.h>
#include <stdexcept>
#include <vector>
#include <set>
#include <string>
#include <optional>
#include <fstream>
#include <chrono>

// Dependencies
#include <SDL2/SDL_vulkan.h> // For surface creation and drawable size
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan clip space
#include <SDL_events.h>
#include <cmake-build-debug/_deps/glm-src/glm/glm.hpp>
#include <cmake-build-debug/_deps/glm-src/glm/gtc/matrix_transform.hpp>

// --- Constants ---
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// List of validation layers to enable (if requested)
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// List of required device extensions
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    // --- Portability Subset Extension (Needed for MoltenVK on macOS) ---
#ifdef __APPLE__
    , "VK_KHR_portability_subset" // Note the comma prefix
#endif
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// --- VK_CHECK Macro ---
#define VK_CHECK(result, msg) \
    if (result != VK_SUCCESS) { \
        spdlog::critical("Vulkan call failed: {} - VkResult: {}", msg, static_cast<int>(result)); \
        throw std::runtime_error(std::string(msg) + " failed!"); \
    }

namespace VkGameProjectOne {
    // --- Static Helper Functions ---

    // Read shader SPIR-V file
    static std::vector<char> readFile(const std::string &filename) {
        // Start at end, read as binary
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            std::string errorMsg = "Failed to open shader file: " + filename;
            spdlog::error(errorMsg);
            throw std::runtime_error(errorMsg);
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        spdlog::debug("Read shader file '{}', size: {} bytes", filename, fileSize);
        return buffer;
    }

    // --- Vulkan Debug Callback Implementation ---
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                spdlog::trace("[Vulkan Debug] {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                spdlog::debug("[Vulkan Debug] {}", pCallbackData->pMessage); // Log info as debug
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                spdlog::warn("[Vulkan Debug] {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                spdlog::error("[Vulkan Debug] {}", pCallbackData->pMessage);
                break;
            default:
                spdlog::info("[Vulkan Debug] (Severity {}) {}", static_cast<int>(messageSeverity),
                             pCallbackData->pMessage);
                break;
        }
        return VK_FALSE; // Don't abort the Vulkan call
    }


    // --- Vertex Struct Implementation ---
    // (Assuming these are already implemented correctly in your VulkanEngine.cpp or a separate file)
    VkVertexInputBindingDescription Vertex::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // layout(location = 0) in vertex shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // Color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1; // layout(location = 1) in vertex shader
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }

    // --- Cube Data ---
    // (Should match your previous definition)
    const std::vector<Vertex> cubeVertices = {
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}}, // 0 Front-Bottom-Left (Red)
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, // 1 Front-Bottom-Right (Green)
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // 2 Front-Top-Right (Blue)
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}}, // 3 Front-Top-Left (Yellow)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 4 Back-Bottom-Left (Magenta)
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 5 Back-Bottom-Right (Cyan)
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // 6 Back-Top-Right (White)
        {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}} // 7 Back-Top-Left (Grey)
    };

    const std::vector<uint16_t> cubeIndices = {
        0, 1, 2, 2, 3, 0, // Front
        1, 5, 6, 6, 2, 1, // Right
        5, 4, 7, 7, 6, 5, // Back
        4, 0, 3, 3, 7, 4, // Left
        3, 2, 6, 6, 7, 3, // Top
        4, 5, 1, 1, 0, 4 // Bottom
    };

    // --- VulkanEngine Constructor / Destructor ---
    VulkanEngine::VulkanEngine(SDL_Window *sdlWindow) : window(sdlWindow) {
        if (!window) {
            spdlog::critical("VulkanEngine requires a valid SDL_Window!");
            throw std::runtime_error("Window pointer passed to VulkanEngine was null!");
        }
        spdlog::info("Initializing VulkanEngine...");
        try {
            initVulkan();
            // initTextRendering(); // Placeholder call for future
        } catch (const std::exception &e) {
            spdlog::critical("Vulkan Engine initialization failed: {}", e.what());
            // Cleanup partially initialized resources if an exception occurs during init
            cleanup(); // Call cleanup to handle partial initialization
            throw; // Re-throw the exception
        }
        spdlog::info("VulkanEngine Initialized Successfully.");
    }

    VulkanEngine::~VulkanEngine() {
        spdlog::info("Destroying VulkanEngine...");
        if (device != VK_NULL_HANDLE) {
            // IMPORTANT: Wait for the GPU to finish all work before destroying resources
            VkResult waitResult = vkDeviceWaitIdle(device);
            if (waitResult != VK_SUCCESS) {
                // Log error but continue cleanup if possible
                spdlog::error("vkDeviceWaitIdle failed during VulkanEngine destruction! VkResult: {}",
                              static_cast<int>(waitResult));
            }
        }
        cleanup();
        // cleanupTextRendering(); // Placeholder call for future
        spdlog::info("VulkanEngine Destroyed.");
    }

    // --- Core Initialization Sequence ---

    void VulkanEngine::initVulkan() {
        spdlog::debug("Starting Vulkan initialization sequence...");
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
        spdlog::debug("Vulkan initialization sequence complete.");
    }

    // --- Resource Creation Methods ---

    void VulkanEngine::createInstance() {
        spdlog::debug("Creating Vulkan instance...");

        if (enableValidationLayers && !checkValidationLayerSupport()) {
            spdlog::error("Validation layers requested, but not available!");
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VkGameProjectOne";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Custom Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3; // Request Vulkan 1.3

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Get required extensions (SDL + Debug + Portability)
        uint32_t sdlExtensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
        std::vector<const char *> requiredExtensions(sdlExtensionCount);
        SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, requiredExtensions.data());

        spdlog::debug("SDL required instance extensions:");
        for (const char *ext: requiredExtensions) spdlog::debug("  - {}", ext);

        if (enableValidationLayers) {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            spdlog::debug("Adding required extension: {}", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

#ifdef __APPLE__
        requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        // Required by MoltenVK >= 1.3.216: Allows requesting features that are not strictly Vulkan conformant
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        spdlog::debug("macOS: Adding required extensions: {} & {}",
                      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        spdlog::debug("macOS: Setting VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag.");
#endif

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        // Enable validation layers
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // Keep this alive if needed for createInstance itself
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            spdlog::debug("Enabling validation layers:");
            for (const char *layer: validationLayers) spdlog::debug("  - {}", layer);

            // Optionally add debug messenger create info to pNext for create/destroy messages
            // populateDebugMessengerCreateInfo(debugCreateInfo);
            // createInfo.pNext = &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // Create instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        VK_CHECK(result, "Failed to create Vulkan instance");

        spdlog::info("Vulkan Instance created successfully.");
    }

    void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional: Can pass pointer to app data
    }

    void VulkanEngine::setupDebugMessenger() {
        if (!enableValidationLayers) return;
        spdlog::debug("Setting up Vulkan debug messenger...");

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
            if (result == VK_SUCCESS) {
                spdlog::info("Vulkan debug messenger created.");
            } else {
                spdlog::warn("Failed to set up debug messenger! VkResult: {}", static_cast<int>(result));
                // Continue without debug messenger if creation fails but layers are enabled
            }
        } else {
            spdlog::warn("vkCreateDebugUtilsMessengerEXT function pointer not found. Cannot create debug messenger.");
        }
    }

    bool VulkanEngine::checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        spdlog::debug("Available validation layers:");
        for (const auto &layerProperties: availableLayers) {
            spdlog::debug("  - {}", layerProperties.layerName);
        }

        for (const char *layerName: validationLayers) {
            bool layerFound = false;
            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                spdlog::warn("Required validation layer not found: {}", layerName);
                return false;
            }
        }
        spdlog::debug("All required validation layers are available.");
        return true;
    }

    void VulkanEngine::createSurface() {
        spdlog::debug("Creating Vulkan surface...");
        // Use SDL helper function via the Window class (conceptual)
        // Assuming Window class has a method like:
        // VkResult Window::createSurface(VkInstance instance, VkSurfaceKHR* surface);
        // (Implementation would call SDL_Vulkan_CreateSurface)

        if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
            std::string errorMsg = "SDL_Vulkan_CreateSurface failed: " + std::string(SDL_GetError());
            spdlog::critical(errorMsg);
            throw std::runtime_error(errorMsg);
        }
        spdlog::info("Vulkan surface created using SDL.");
    }

    // --- Device Selection ---
    QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice queryDevice) const {
        VkGameProjectOne::QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(queryDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(queryDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            // Check for graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
                spdlog::trace("Found graphics queue family: index {}", i);
            }

            // Check for presentation support
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(queryDevice, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
                spdlog::trace("Found present queue family: index {}", i);
            }

            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    }

    bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice queryDevice) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(queryDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(queryDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());

        spdlog::trace("Device supports extensions:");
        for (const auto &extension: availableExtensions) {
            spdlog::trace("  - {}", extension.extensionName);
            required.erase(extension.extensionName); // Remove found extension from required set
        }

        if (!required.empty()) {
            spdlog::warn("Device is missing required extensions:");
            for (const auto &req: required) spdlog::warn("  - {}", req);
        }

        return required.empty(); // True if all required extensions were found
    }

    VulkanEngine::SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice queryDevice) const {
        SwapChainSupportDetails details;
        // Get capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(queryDevice, surface, &details.capabilities);

        // Get formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(queryDevice, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(queryDevice, surface, &formatCount, details.formats.data());
        }

        // Get present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(queryDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(queryDevice, surface, &presentModeCount,
                                                      details.presentModes.data());
        }
        return details;
    }


    bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice queryDevice) {
        spdlog::debug("Checking suitability of device...");
        // Basic properties check (optional, could check name/type)
        // VkPhysicalDeviceProperties deviceProperties;
        // vkGetPhysicalDeviceProperties(queryDevice, &deviceProperties);
        // spdlog::debug("  Device Name: {}", deviceProperties.deviceName);

        // Check for required queue families
        VkGameProjectOne::QueueFamilyIndices indices = findQueueFamilies(queryDevice);
        if (!indices.isComplete()) {
            spdlog::warn("  Device missing required queue families.");
            return false;
        }

        // Check for required extensions
        bool extensionsSupported = checkDeviceExtensionSupport(queryDevice);
        if (!extensionsSupported) {
            spdlog::warn("  Device missing required extensions.");
            return false;
        }

        // Check for adequate swap chain support (at least one format and present mode)
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(queryDevice);
        bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if (!swapChainAdequate) {
            spdlog::warn("  Device does not provide adequate swap chain support (formats/present modes).");
            return false;
        }

        // Check for required features (optional, e.g., samplerAnisotropy)
        // VkPhysicalDeviceFeatures supportedFeatures;
        // vkGetPhysicalDeviceFeatures(queryDevice, &supportedFeatures);
        // if (!supportedFeatures.samplerAnisotropy) return false;

        spdlog::debug("  Device is suitable.");
        return true;
    }

    void VulkanEngine::pickPhysicalDevice() {
        spdlog::debug("Picking physical device...");
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            spdlog::critical("Failed to find GPUs with Vulkan support!");
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        spdlog::info("Found {} Vulkan-capable device(s):", deviceCount);
        int suitableDeviceIndex = -1;
        int currentDeviceIndex = 0;
        for (const auto &device: devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            spdlog::info("  Device [{}]: {}", currentDeviceIndex, deviceProperties.deviceName);
            if (isDeviceSuitable(device)) {
                physicalDevice = device; // Select the first suitable device
                suitableDeviceIndex = currentDeviceIndex;
                spdlog::info("    -> Selected as suitable device.");
                // Optional: Prefer discrete GPUs
                // if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                //     break; // Found preferred type
                // }
                break; // Select the first suitable one found
            }
            currentDeviceIndex++;
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            spdlog::critical("Failed to find a suitable GPU!");
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
        spdlog::info("Physical device selected: index {}", suitableDeviceIndex);
    }


    void VulkanEngine::createLogicalDevice() {
        spdlog::debug("Creating logical device...");
        VkGameProjectOne::QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        // Use a set to handle cases where graphics and present families are the same
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        float queuePriority = 1.0f; // Priority between 0.0 and 1.0
        for (uint32_t queueFamilyIndex: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = 1; // Request one queue from the family
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
            spdlog::debug("Requesting queue from family index: {}", queueFamilyIndex);
        }

        // Specify device features we want to use (start with none)
        VkPhysicalDeviceFeatures deviceFeatures{};
        // deviceFeatures.samplerAnisotropy = VK_TRUE; // Example feature

        // Enable portability subset feature if needed (required by MoltenVK)
        VkPhysicalDevicePortabilitySubsetFeaturesKHR portabilityFeatures = {};
#ifdef __APPLE__
        portabilityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
        // List specific features from the portability subset if needed, e.g.:
        // portabilityFeatures.constantAlphaColorBlendFactors = VK_TRUE;
#endif


        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures; // Link features structure

        // Link portability features struct on macOS
#ifdef __APPLE__
        portabilityFeatures.pNext = const_cast<void *>(createInfo.pNext); // Chain structs
        createInfo.pNext = &portabilityFeatures;
#endif

        // Enable required device extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        spdlog::debug("Enabling device extensions:");
        for (const char *ext: deviceExtensions) spdlog::debug("  - {}", ext);

        // Enable validation layers (consistent with instance)
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        // Create the logical device
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        VK_CHECK(result, "Failed to create logical device!");

        // Get queue handles (we requested one queue per family)
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        spdlog::info("Logical device created.");
        spdlog::debug("Retrieved graphics queue (family {}) and present queue (family {}).",
                      indices.graphicsFamily.value(), indices.presentFamily.value());
    }


    // --- Swapchain Creation Helpers ---

    VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        // Prefer SRGB format if available for better color representation
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                spdlog::debug("Chosen swap surface format: B8G8R8A8_SRGB, SRGB_NONLINEAR");
                return availableFormat;
            }
        }
        // Otherwise, return the first available format
        spdlog::debug("Chosen swap surface format: format {}, colorspace {}",
                      static_cast<int>(availableFormats[0].format), static_cast<int>(availableFormats[0].colorSpace));
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        // Prefer Mailbox (triple buffering) for lowest latency without tearing
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                spdlog::debug("Chosen swap present mode: MAILBOX_KHR (Triple Buffering)");
                return availablePresentMode;
            }
        }
        // FIFO is guaranteed to be available (standard VSync)
        spdlog::debug("Chosen swap present mode: FIFO_KHR (VSync)");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        // If currentExtent is defined, use it
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            spdlog::debug("Chosen swap extent: {}x{} (from capabilities.currentExtent)",
                          static_cast<int>(capabilities.currentExtent.width),
                          static_cast<int>(capabilities.currentExtent.height));
            return capabilities.currentExtent;
        } else {
            // Otherwise, get size from SDL window and clamp to capabilities
            int width, height;
            SDL_Vulkan_GetDrawableSize(window, &width, &height); // Use SDL func for high-DPI awareness
            spdlog::debug("Window drawable size: {}x{}", width, height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Clamp width and height between min and max extents supported
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            spdlog::debug("Chosen swap extent: {}x{} (clamped from window size)", static_cast<int>(actualExtent.width),
                          static_cast<int>(actualExtent.height));
            return actualExtent;
        }
    }

    // --- Swapchain Creation ---

    void VulkanEngine::createSwapChain() {
        spdlog::debug("Creating swap chain...");
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // Determine number of images in swap chain (request one more than min for triple buffering potential)
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        // Clamp image count if maxImageCount is specified (0 means no limit other than memory)
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.
            maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        spdlog::debug("Requesting swap chain image count: {}", imageCount);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Usually 1 unless doing stereoscopic rendering
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Draw directly to images

        // Handle queue families (concurrent vs exclusive)
        VkGameProjectOne::QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            // Images used by multiple queues need concurrent access
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
            spdlog::debug("Swap chain image sharing mode: CONCURRENT (graphics queue != present queue)");
        } else {
            // Images used by single queue family
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
            spdlog::debug("Swap chain image sharing mode: EXCLUSIVE (graphics queue == present queue)");
        }

        // Specify pre-transform (usually current transform)
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        // Specify alpha compositing mode (usually opaque)
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // Allow clipping of obscured pixels

        // Handle old swapchain if recreating
        createInfo.oldSwapchain = VK_NULL_HANDLE; // Set later if recreating

        // Create the swapchain
        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
        VK_CHECK(result, "Failed to create swap chain!");

        // Retrieve swap chain images
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr); // Get actual count
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        spdlog::info("Swap chain created with {} images.", static_cast<int>(imageCount));

        // Store format and extent for later use
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void VulkanEngine::createImageViews() {
        spdlog::debug("Creating swap chain image views...");
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            // Component mapping (identity mapping)
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // Subresource range (color target, no mipmapping, single layer)
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
            VK_CHECK(result, "Failed to create image view for swap chain image " + std::to_string(i));
        }
        spdlog::debug("Created {} swap chain image views.", static_cast<int>(swapChainImageViews.size()));
    }

    // --- Render Pass ---

    void VulkanEngine::createRenderPass() {
        spdlog::debug("Creating render pass...");
        // Define the color attachment (matches swapchain image format)
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling yet
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear framebuffer before drawing
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store result to be presented
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before render pass
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout after render pass (for presentation)

        // Attachment reference for the subpass
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // Index in the pAttachments array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during subpass

        // Define the subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // No depth, input, resolve, or preserve attachments for now

        // Subpass dependency (ensures render pass waits for image acquisition)
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit subpass before render pass
        dependency.dstSubpass = 0; // Our first (and only) subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage producing the output
        dependency.srcAccessMask = 0; // Access mask for the source stage
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage consuming the input
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Access required by destination stage

        // Define the render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
        VK_CHECK(result, "Failed to create render pass!");
        spdlog::info("Render pass created.");
    }


    // --- Descriptor Sets (for Uniform Buffer) ---

    void VulkanEngine::createDescriptorSetLayout() {
        spdlog::debug("Creating descriptor set layout...");
        // Define layout binding for the Uniform Buffer Object (UBO)
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0; // Binding point used in the shader (layout(binding = 0))
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1; // Number of descriptors in the binding (just one UBO)
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // UBO is used in the vertex shader
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Create the descriptor set layout
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        VK_CHECK(result, "Failed to create descriptor set layout!");
        spdlog::info("Descriptor set layout created.");
    }


    // --- Graphics Pipeline ---

    // Helper to create shader modules
    VkShaderModule VulkanEngine::createShaderModule(const std::vector<char> &code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // Note: pCode requires uint32_t*, need careful casting
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
        VK_CHECK(result, "Failed to create shader module!");
        spdlog::debug("Shader module created (size: {} bytes)", code.size());
        return shaderModule;
    }

    void VulkanEngine::createGraphicsPipeline() {
        spdlog::debug("Creating graphics pipeline...");
        // Load shader code
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        // Create shader modules
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // Define shader stages
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // Entry point function name

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // --- Fixed Function State ---

        // Vertex Input
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input Assembly (Triangles)
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and Scissor (Dynamic state allows changing without rebuilding pipeline)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        // pViewports and pScissors set dynamically via command buffer

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // Clamping fragments beyond near/far planes
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // Don't discard geometry
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill polygons
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back faces
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Define front face (due to proj matrix Y-flip)
        rasterizer.depthBiasEnable = VK_FALSE;
        // rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        // rasterizer.depthBiasClamp = 0.0f; // Optional
        // rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling (Disabled for now)
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        // multisampling.minSampleShading = 1.0f; // Optional
        // multisampling.pSampleMask = nullptr; // Optional
        // multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        // multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth/Stencil Testing (Disabled for now)
        // VkPipelineDepthStencilStateCreateInfo depthStencil{};
        // ...

        // Color Blending (Simple alpha blending, disabled here)
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // No blending
        // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; // Use blend factors, not bitwise logic op
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        // colorBlending.blendConstants[0] = 0.0f; // Optional
        // colorBlending.blendConstants[1] = 0.0f; // Optional
        // colorBlending.blendConstants[2] = 0.0f; // Optional
        // colorBlending.blendConstants[3] = 0.0f; // Optional

        // Pipeline Layout (Specifies uniforms/push constants)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Use the UBO layout
        // pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        // pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        VkResult layoutResult = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        VK_CHECK(layoutResult, "Failed to create pipeline layout!");
        spdlog::debug("Pipeline layout created.");

        // --- Create Graphics Pipeline ---
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2; // Vertex + Fragment stages
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        // pipelineInfo.pDepthStencilState = nullptr; // No depth testing yet
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState; // Enable dynamic viewport/scissor
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass; // Render pass the pipeline is compatible with
        pipelineInfo.subpass = 0; // Index of the subpass
        // pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional: For pipeline derivation
        // pipelineInfo.basePipelineIndex = -1; // Optional

        VkResult pipelineResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                                            &graphicsPipeline);
        VK_CHECK(pipelineResult, "Failed to create graphics pipeline!");
        spdlog::info("Graphics pipeline created.");

        // Cleanup shader modules after pipeline creation
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        spdlog::debug("Shader modules destroyed.");
    }

    // --- Framebuffers ---

    void VulkanEngine::createFramebuffers() {
        spdlog::debug("Creating framebuffers...");
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass; // Compatible render pass
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments; // Image view for the attachment
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
            VK_CHECK(result, "Failed to create framebuffer " + std::to_string(i));
        }
        spdlog::debug("Created {} framebuffers.", swapChainFramebuffers.size());
    }

    // --- Command Pool ---

    void VulkanEngine::createCommandPool() {
        spdlog::debug("Creating command pool...");
        VkGameProjectOne::QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow resetting individual command buffers
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(); // Pool for graphics commands

        VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
        VK_CHECK(result, "Failed to create command pool!");
        spdlog::info("Command pool created for graphics queue family {}.", queueFamilyIndices.graphicsFamily.value());
    }


    // --- Buffer Creation Helpers ---

    uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // Check if the memory type is suitable (matches the type filter bit)
            // AND if it has the required properties (matches all property flags)
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                spdlog::trace("Found suitable memory type: index {}", i);
                return i;
            }
        }

        spdlog::critical("Failed to find suitable memory type!");
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void VulkanEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                    VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
        spdlog::trace("Creating buffer (size: {}, usage: {}, properties: {})", size, usage, properties);
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assume exclusive for simplicity

        VkResult createResult = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
        VK_CHECK(createResult, "Failed to create buffer");

        // Allocate memory for the buffer
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VkResult allocResult = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
        VK_CHECK(allocResult, "Failed to allocate buffer memory");

        // Bind the memory to the buffer
        VkResult bindResult = vkBindBufferMemory(device, buffer, bufferMemory, 0); // Bind at offset 0
        VK_CHECK(bindResult, "Failed to bind buffer memory");
        spdlog::trace("Buffer created and memory bound successfully.");
    }

    // Helper to execute short-lived commands (like buffer copies)
    VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer),
                 "Failed to allocate single time command buffer");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Use buffer only once

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin single time command buffer");
        return commandBuffer;
    }

    void VulkanEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to end single time command buffer");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Submit to graphics queue and wait for completion
        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        VK_CHECK(submitResult, "Failed to submit single time command buffer");

        VkResult waitResult = vkQueueWaitIdle(graphicsQueue); // Wait for copy to finish
        VK_CHECK(waitResult, "Failed to wait for queue idle after single time command");

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer); // Free the temporary buffer
    }


    void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        spdlog::trace("Copying buffer ({} bytes)...", size);
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
        spdlog::trace("Buffer copy complete.");
    }

    // --- Vertex and Index Buffers ---

    void VulkanEngine::createVertexBuffer() {
        spdlog::debug("Creating vertex buffer...");
        VkDeviceSize bufferSize = sizeof(cubeVertices[0]) * cubeVertices.size();
        spdlog::debug("  Vertex data size: {} bytes", bufferSize);

        // Create staging buffer (CPU accessible)
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map staging buffer memory and copy vertex data
        void *data;
        VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data),
                 "Failed to map vertex staging buffer memory");
        memcpy(data, cubeVertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory); // Unmap after copy (or keep mapped if needed often)
        spdlog::trace("  Vertex data copied to staging buffer.");

        // Create vertex buffer (GPU local)
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer, vertexBufferMemory);

        // Copy data from staging buffer to vertex buffer
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // Clean up staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        spdlog::info("Vertex buffer created (Device Local).");
    }

    void VulkanEngine::createIndexBuffer() {
        spdlog::debug("Creating index buffer...");
        VkDeviceSize bufferSize = sizeof(cubeIndices[0]) * cubeIndices.size();
        spdlog::debug("  Index data size: {} bytes", bufferSize);

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map and copy index data
        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, cubeIndices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
        spdlog::trace("  Index data copied to staging buffer.");

        // Index buffer (GPU local)
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer, indexBufferMemory);

        // Copy from staging to index buffer
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        // Cleanup staging
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        spdlog::info("Index buffer created (Device Local).");
    }

    // --- Uniform Buffers ---

    void VulkanEngine::createUniformBuffers() {
        spdlog::debug("Creating uniform buffers...");
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT); // Store mapped pointers

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         // Host Visible: CPU can see it, Host Coherent: No manual flushing needed
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i], uniformBuffersMemory[i]);

            // Map the buffer persistently
            VkResult mapResult = vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                                             &uniformBuffersMapped[i]);
            VK_CHECK(mapResult, "Failed to map uniform buffer memory for frame " + std::to_string(i));
            spdlog::trace("  Uniform buffer {} created and persistently mapped.", i);
        }
        spdlog::debug("Created {} uniform buffers.", MAX_FRAMES_IN_FLIGHT);
    }

    // --- Descriptor Pool and Sets ---

    void VulkanEngine::createDescriptorPool() {
        spdlog::debug("Creating descriptor pool...");
        // Describe the types and counts of descriptors needed
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // One UBO descriptor per frame

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // Max number of sets to allocate

        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        VK_CHECK(result, "Failed to create descriptor pool!");
        spdlog::info("Descriptor pool created.");
    }

    void VulkanEngine::createDescriptorSets() {
        spdlog::debug("Creating descriptor sets...");
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        // Use same layout for all sets
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
        VK_CHECK(result, "Failed to allocate descriptor sets!");

        // Update each descriptor set to point to its corresponding uniform buffer
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i]; // Buffer for this frame
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i]; // Set to update
            descriptorWrite.dstBinding = 0; // Binding 0 (matches layout)
            descriptorWrite.dstArrayElement = 0; // Not an array
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo; // Link buffer info
            // descriptorWrite.pImageInfo = nullptr; // Optional
            // descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr); // Update the set
            spdlog::trace("  Descriptor set {} updated to point to uniform buffer {}.", i, i);
        }
        spdlog::debug("Created and updated {} descriptor sets.", MAX_FRAMES_IN_FLIGHT);
    }

    // --- Command Buffers ---

    void VulkanEngine::createCommandBuffers() {
        spdlog::debug("Creating command buffers...");
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // Primary buffers can be submitted directly to a queue
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
        VK_CHECK(result, "Failed to allocate command buffers!");
        spdlog::debug("Allocated {} primary command buffers.", commandBuffers.size());
    }

    // --- Synchronization Objects ---

    void VulkanEngine::createSyncObjects() {
        spdlog::debug("Creating synchronization objects (semaphores/fences)...");
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        // Create fences in signaled state (so first frame doesn't wait indefinitely)

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkResult iasResult = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
            VkResult rfsResult = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
            VkResult iffResult = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);

            if (iasResult != VK_SUCCESS || rfsResult != VK_SUCCESS || iffResult != VK_SUCCESS) {
                spdlog::critical("Failed to create synchronization objects for frame {}", i);
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
        spdlog::debug("Created {} sets of semaphores and fences.", MAX_FRAMES_IN_FLIGHT);
    }

    // --- Command Buffer Recording ---

    void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        // spdlog::trace("Recording command buffer for frame {}, image index {}", currentFrame, imageIndex); // Can be noisy
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // beginInfo.flags = 0;
        // beginInfo.pInheritanceInfo = nullptr;

        VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VK_CHECK(beginResult, "Failed to begin recording command buffer!");

        // Begin Render Pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;
        VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}}; // Clear color
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Set dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // Set dynamic scissor
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        // Bind index buffer
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // Bind descriptor set for the current frame
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                0, 1, &descriptorSets[currentFrame], 0, nullptr);

        // Draw indexed command
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(cubeIndices.size()), 1, 0, 0, 0);

        // Draw Text (Placeholder)
        // drawText(commandBuffer);

        // End render pass
        vkCmdEndRenderPass(commandBuffer);

        // End recording
        VkResult endResult = vkEndCommandBuffer(commandBuffer);
        VK_CHECK(endResult, "Failed to record command buffer!");
    }


    // --- Update and Drawing Logic ---

    // Renamed from updateUniformBuffer to reflect purpose
    void VulkanEngine::updateCubeRotation(float time) {
        // This now calculates AND copies the UBO data
        UniformBufferObject ubo{};
        // Rotate model around Y axis
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Set up view matrix (camera)
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), // Eye position
                               glm::vec3(0.0f, 0.0f, 0.0f), // Target position
                               glm::vec3(0.0f, 1.0f, 0.0f)); // Up direction (Y-up)
        // Set up projection matrix
        ubo.proj = glm::perspective(glm::radians(45.0f), // Field of view
                                    (float) swapChainExtent.width / (float) swapChainExtent.height, // Aspect ratio
                                    0.1f, // Near plane
                                    10.0f); // Far plane
        // Adjust for Vulkan clip space (Y coordinate flipped)
        ubo.proj[1][1] *= -1;

        // Copy data to the mapped buffer for the current frame in flight
        if (uniformBuffersMapped.size() > currentFrame && uniformBuffersMapped[currentFrame]) {
            memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
        } else {
            // This shouldn't happen if initialization is correct
            spdlog::error("Attempted to update uniform buffer for frame {}, but it's not mapped!", currentFrame);
        }
    }


    void VulkanEngine::drawFrame() {
        // spdlog::trace("drawFrame start (frame {})", currentFrame); // Can be noisy

        // 1. Wait for the previous frame (using fence for the current frame index) to finish
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        // Note: Fence was reset just after vkAcquireNextImageKHR in the previous successful submission

        // 2. Acquire an image from the swap chain
        uint32_t imageIndex; // Index of the swap chain image that is available
        VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                       imageAvailableSemaphores[currentFrame],
                                                       // Semaphore to signal when image is available
                                                       VK_NULL_HANDLE, // Fence to signal (optional)
                                                       &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            spdlog::warn("Swap chain out of date during image acquisition. Recreating.");
            recreateSwapChain(); // Recreate swapchain and dependent resources
            throw SwapChainOutOfDateError(); // Signal loop to potentially skip rest of frame logic
        } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            // Suboptimal means it still works but presentation might not be perfect
            spdlog::critical("Failed to acquire swap chain image! VkResult: {}", static_cast<int>(acquireResult));
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        // Image acquired successfully (or suboptimal)
        // NOW we can reset the fence, because we know we are going to submit work for this frame
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        // 3. Update Uniform Buffer (now handled by Application::mainLoop calling updateCubeRotation)
        // updateUniformBuffer(currentFrame); // Call this if update isn't external

        // 4. Record the command buffer for the acquired image index
        vkResetCommandBuffer(commandBuffers[currentFrame], 0); // Reset before recording
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex); // Pass acquired image index

        // 5. Submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // Wait for image available semaphore before executing color output stage
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        // Command buffer to execute
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        // Semaphore to signal when rendering finishes
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Submit to graphics queue, signal fence when done
        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        VK_CHECK(submitResult, "Failed to submit draw command buffer!");

        // 6. Presentation
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // Wait for rendering to finish before presentation
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        // Specify swapchain and image index to present
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        // presentInfo.pResults = nullptr; // Optional: Check results for multiple swapchains

        VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
            spdlog::warn("Swap chain out of date or suboptimal during presentation, or window resized. Recreating.");
            framebufferResized = false; // Reset flag after handling
            recreateSwapChain();
            // No exception needed here, next frame's acquire will handle OOD properly
        } else if (presentResult != VK_SUCCESS) {
            spdlog::critical("Failed to present swap chain image! VkResult: {}", static_cast<int>(presentResult));
            throw std::runtime_error("Failed to present swap chain image!");
        }

        // 7. Advance frame counter
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        // spdlog::trace("drawFrame end (advancing to frame {})", currentFrame); // Can be noisy
    }


    // --- Swapchain Recreation ---

    void VulkanEngine::cleanupSwapChain() {
        spdlog::debug("Cleaning up swap chain resources...");

        // Destroy Framebuffers
        for (auto framebuffer: swapChainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        swapChainFramebuffers.clear();
        // Destroy Graphics Pipeline
        if (graphicsPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
        // Destroy Pipeline Layout
        if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
        // Destroy Render Pass
        if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
        // Destroy Image Views
        for (auto imageView: swapChainImageViews) vkDestroyImageView(device, imageView, nullptr);
        swapChainImageViews.clear();
        // Destroy Swapchain itself
        if (swapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
        // Destroy Uniform Buffers & Memory & Mapped pointers
        for (size_t i = 0; i < uniformBuffers.size(); ++i) {
            if (uniformBuffersMapped.size() > i && uniformBuffersMapped[i]) {
                // vkUnmapMemory(device, uniformBuffersMemory[i]); // No need to unmap HOST_COHERENT
                uniformBuffersMapped[i] = nullptr;
            }
            if (uniformBuffers[i] != VK_NULL_HANDLE) vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            if (uniformBuffersMemory.size() > i && uniformBuffersMemory[i] != VK_NULL_HANDLE)
                vkFreeMemory(
                    device, uniformBuffersMemory[i], nullptr);
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();
        // Destroy Descriptor Pool (simpler to recreate pool and sets)
        if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
        // Descriptor sets are implicitly freed with the pool

        spdlog::debug("Swap chain resource cleanup finished.");
    }

    void VulkanEngine::recreateSwapChain() {
        spdlog::info("Recreating swap chain...");

        // Handle minimization: pause rendering until window has size > 0
        int width = 0, height = 0;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        while (width == 0 || height == 0) {
            spdlog::debug("Window minimized, waiting for resize...");
            SDL_Vulkan_GetDrawableSize(window, &width, &height);
            SDL_WaitEvent(nullptr); // Wait for any SDL event
        }
        spdlog::debug("Window has size {}x{}, proceeding with swap chain recreation.", width, height);

        // Wait for current operations to complete before destroying old resources
        VkResult waitResult = vkDeviceWaitIdle(device);
        if (waitResult != VK_SUCCESS) {
            spdlog::error("vkDeviceWaitIdle failed before swapchain recreation! VkResult: {}",
                          static_cast<int>(waitResult));
            // Attempt to continue, but things might be unstable
        }

        cleanupSwapChain(); // Destroy old swapchain and dependent objects

        // Recreate swapchain and dependent objects
        createSwapChain();
        createImageViews();
        createRenderPass(); // Might need changes if multisampling/depth added
        createGraphicsPipeline(); // Depends on renderpass, extent etc.
        createFramebuffers();
        createUniformBuffers(); // Depends on number of swapchain images / frames in flight
        createDescriptorPool(); // Recreate pool
        createDescriptorSets(); // Recreate and bind sets
        // Command buffers are usually okay unless render pass compatibility changes,
        // but they will be re-recorded anyway in drawFrame.

        spdlog::info("Swap chain recreated successfully.");
    }


    // --- Main Cleanup Method ---

    void VulkanEngine::cleanup() {
        spdlog::debug("Starting main VulkanEngine cleanup...");
        // Cleanup swapchain first (calls vkDeviceWaitIdle implicitly via recreate or explicitly in destructor)
        cleanupSwapChain(); // Ensures swapchain resources are gone first

        // Destroy objects created before swapchain dependencies
        if (descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;

        if (indexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
        if (indexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;

        if (vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
        if (vertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;

        // Destroy sync objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            // Check vector size before accessing index
            if (renderFinishedSemaphores.size() > i && renderFinishedSemaphores[i])
                vkDestroySemaphore(
                    device, renderFinishedSemaphores[i], nullptr);
            if (imageAvailableSemaphores.size() > i && imageAvailableSemaphores[i])
                vkDestroySemaphore(
                    device, imageAvailableSemaphores[i], nullptr);
            if (inFlightFences.size() > i && inFlightFences[i]) vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        renderFinishedSemaphores.clear();
        imageAvailableSemaphores.clear();
        inFlightFences.clear();

        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE; // Command buffers freed with pool

        // Destroy logical device LAST before instance-level objects
        if (device != VK_NULL_HANDLE) vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;

        // Destroy instance-level objects
        if (debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) func(instance, debugMessenger, nullptr);
            debugMessenger = VK_NULL_HANDLE;
        }
        if (surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;

        spdlog::debug("Main VulkanEngine cleanup finished.");
    }

    // --- Placeholder Text Rendering Methods ---
    void VulkanEngine::initTextRendering() {
        spdlog::info("[TODO] Initialize Text Rendering...");
    }

    void VulkanEngine::drawText(VkCommandBuffer commandBuffer) {
        spdlog::trace("[TODO] Record Text Drawing Commands...");
    }

    void VulkanEngine::cleanupTextRendering() {
        spdlog::info("[TODO] Cleanup Text Rendering Resources...");
    }
} // namespace VkGameProjectOne
