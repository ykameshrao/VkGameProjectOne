
// Window.cpp

#include "core/Window.h"
#include <cmake-build-debug/_deps/spdlog-src/include/spdlog/spdlog.h>
#include <stdexcept>

namespace vk_project_one {
    // Use new namespace

    Window::Window(const int width, const int height, std::string title) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            const std::string errorMsg = "SDL_Init failed: " + std::string(SDL_GetError());
            spdlog::critical(errorMsg);
            throw std::runtime_error(errorMsg);
        }
        spdlog::debug("SDL Initialized. Creating window...");

        if (SDL_Vulkan_LoadLibrary(nullptr) == -1) {
            const std::string errorMsg = "SDL_Vulkan_LoadLibrary failed: " + std::string(SDL_GetError());
            spdlog::critical(errorMsg);
            throw std::runtime_error(errorMsg);
        }

        sdlWindow = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
        );

        if (sdlWindow == nullptr) {
            const char *sdlError = SDL_GetError();
            std::string errorMsg = "SDL_CreateWindow failed";
            if (sdlError && strlen(sdlError) > 0 && strcmp(sdlError, "Unknown error") != 0) {
                errorMsg += ": ";
                errorMsg += sdlError;
            } else {
                errorMsg += " (No specific error message from SDL)";
            }
            spdlog::critical(errorMsg);
            SDL_Quit();
            throw std::runtime_error(errorMsg);
        }
        spdlog::info("Window created: '{}'", title);
    }

    Window::~Window() {
        spdlog::debug("Destroying Window...");
        if (sdlWindow != nullptr) SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        spdlog::info("Window destroyed.");
    }

    VkResult Window::createSurface(VkInstance instance, VkSurfaceKHR *surface) const {
        if (!SDL_Vulkan_CreateSurface(sdlWindow, instance, surface)) {
            spdlog::error("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        spdlog::debug("Vulkan Surface created via SDL.");
        return VK_SUCCESS;
    }

    void Window::getFramebufferSize(int *width, int *height) const {
        SDL_Vulkan_GetDrawableSize(sdlWindow, width, height);
    }
} // namespace VkGameProjectOne
