
// Window.h

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <string>
#include <vulkan/vulkan.h>

namespace VkGameProjectOne {
    // Use new namespace

    class Window {
    public:
        Window(int width, int height, std::string title);

        ~Window();

        Window(const Window &) = delete;

        Window &operator=(const Window &) = delete;

        VkResult createSurface(VkInstance instance, VkSurfaceKHR *surface) const;

        SDL_Window *getSdlWindow() const { return sdlWindow; }

        void getFramebufferSize(int *width, int *height) const;

    private:
        SDL_Window *sdlWindow = nullptr;
        // ... other members ...
    };
} // namespace VkGameProjectOne
