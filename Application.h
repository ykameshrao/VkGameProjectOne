
// Application.h

#pragma once
#include "Window.h"       // Forward declare or include full header
#include "VulkanEngine.h" // Forward declare or include full header
#include <memory>

// Use the new project namespace
namespace VkGameProjectOne {

class Application {
public:
    Application();
    ~Application();
    void run() const;

private:
    std::unique_ptr<Window> window{};
    std::unique_ptr<VulkanEngine> vulkanEngine{};
    void mainLoop() const;
};

} // namespace VkGameProjectOne