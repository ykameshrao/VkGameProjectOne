
// Application.h

#pragma once
#include "core/Window.h"
#include "core/VulkanEngine.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <chrono>

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