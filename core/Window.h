
// Window.h

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <string>

namespace vk_project_one {
    // Use new namespace

    class Window {
    public:
        Window(int width, int height, std::string title);

        ~Window();

        Window(const Window &) = delete;

        Window &operator=(const Window &) = delete;

        SDL_Window *getSdlWindow() const { return sdlWindow; }

        void getFramebufferSize(int *width, int *height) const;

    private:
        SDL_Window *sdlWindow = nullptr;
        // ... other members ...
    };
} // namespace VkGameProjectOne
