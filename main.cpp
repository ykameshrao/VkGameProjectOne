
// main.cpp

#include "Application.h" // App class within VkGameProjectOne namespace
#include "common/Log.h"         // Logging setup helper
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char *argv[]) {
    // Initialize logging FIRST
    common::InitializeLogging("VkGameProjectOne");
    spdlog::info("Starting VkGameProjectOne Application...");

    try {
        VkGameProjectOne::Application app;
        app.run();
    } catch (const std::exception &e) {
        spdlog::critical("Unhandled exception caught in main: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::critical("Caught unknown exception in main!");
        return EXIT_FAILURE;
    }

    spdlog::info("VkGameProjectOne finished successfully.");
    return EXIT_SUCCESS;
}
