
// Log.h

#pragma once
#include <spdlog/spdlog.h>

// Use the new project namespace
namespace common {
    void InitializeLogging(const std::string& appName);
}