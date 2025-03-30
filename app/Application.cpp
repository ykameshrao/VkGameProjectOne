
// Application.cpp

#include "Application.h"

namespace VkGameProjectOne {
    Application::Application() {
        spdlog::info("Initializing Application...");
        window = std::make_unique<Window>(800, 600, "VkGameProjectOne v0.1");
        vulkanEngine = std::make_unique<VulkanEngine>(window->getSdlWindow());
        spdlog::info("Application Initialized.");
    }

    Application::~Application() {
        spdlog::info("Destroying Application...");
        vulkanEngine.reset();
        window.reset();
        spdlog::info("Application Destroyed.");
    }

    void Application::run() const {
        mainLoop();
    }

    void Application::mainLoop() const {
        bool quit = false;
        SDL_Event e;
        const auto startTime = std::chrono::high_resolution_clock::now();

        while (!quit) {
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                    quit = true;
                }
                if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    spdlog::debug("Window resize event detected.");
                    vulkanEngine->notifyFramebufferResized();
                }
            }

            auto currentTime = std::chrono::high_resolution_clock::now();
            const float time = std::chrono::duration<float>(currentTime - startTime).count();
            vulkanEngine->updateCubeRotation(time); // Update UBO based on time

            try {
                vulkanEngine->drawFrame();
            } catch (const VulkanEngine::SwapChainOutOfDateError &ood_err) {
                spdlog::warn("{}", ood_err.what());
            } catch (const std::exception &draw_err) {
                spdlog::error("Error during drawFrame: {}", draw_err.what());
                quit = true;
            }
        }
    }
} // namespace VkGameProjectOne
