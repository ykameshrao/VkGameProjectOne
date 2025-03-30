
// main.cpp

#include "app/Application.h"
#include "common/Log.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char *argv[]) {
    common::InitializeLogging("VkProjectOne");
    spdlog::info("Starting VkProjectOne Application...");

    try {
        const vk_project_one::Application app;
        app.run();
    } catch (const std::exception &e) {
        spdlog::critical("Unhandled exception caught in main: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::critical("Caught unknown exception in main!");
        return EXIT_FAILURE;
    }

    spdlog::info("VkProjectOne finished successfully.");
    return EXIT_SUCCESS;
}
