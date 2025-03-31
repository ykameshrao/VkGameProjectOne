
// Window.cpp

#include "core/Window.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace vk_project_one {
    Window::Window(const int width, const int height, std::string title) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            const std::string errorMsg = "SDL_Init failed: " + std::string(SDL_GetError());
            spdlog::critical(errorMsg);
            throw std::runtime_error(errorMsg);
        }

        if (SDL_Vulkan_LoadLibrary(nullptr) == -1) {
            const std::string errorMsg = "SDL_Vulkan_LoadLibrary failed: " + std::string(SDL_GetError());
            spdlog::critical(errorMsg);
            throw std::runtime_error(errorMsg);
        }

        spdlog::debug("SDL Initialized. Creating window...");
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title.c_str());
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

        sdlWindow = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);

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

    void Window::getFramebufferSize(int *width, int *height) const {
        if (SDL_GetWindowSizeInPixels(sdlWindow, width, height) != 0) {
            spdlog::error("SDL_GetWindowSizeInPixels failed: {}", SDL_GetError());
            if (width) *width = 0;
            if (height) *height = 0;
        }
    }
} // namespace VkGameProjectOne
