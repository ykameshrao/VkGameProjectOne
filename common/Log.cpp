
// Log.cpp

#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace common {
    void InitializeLogging(const std::string& appName) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        const auto logger = std::make_shared<spdlog::logger>(appName, console_sink);

        register_logger(logger);
        set_default_logger(logger);

#ifdef NDEBUG
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info); // Flush on info level in release
#else
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::trace);
#endif

        spdlog::info("Logging initialized for {}.", appName);
    }

} // namespace common