cmake_minimum_required(VERSION 3.30.5)

project(VkGameProjectOne LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- FetchContent for Dependencies ---
include(FetchContent)

# spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(spdlog)

# glm
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm) # This makes GLM available

# --- Find Standard Dependencies ---
find_package(SDL2 REQUIRED)
message(STATUS "Found SDL2: ${SDL2_LIBRARIES}")

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")

# --- Executable Target ---
add_executable(VkGameProjectOne
        main.cpp
        common/Log.cpp
        app/Application.cpp
        core/Window.cpp
        core/VulkanEngine.cpp
)

# --- Shader Copying ---
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compiled)
set(SHADER_DEST_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_DEST_DIR})
message(STATUS "Ensured shader destination directory exists: ${SHADER_DEST_DIR}")

file(GLOB SHADER_FILES "${SHADER_SOURCE_DIR}/*.spv")
message(STATUS "Found shader files to copy: ${SHADER_FILES}")

foreach (SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_FILENAME ${SHADER_FILE} NAME)
    configure_file(
            ${SHADER_FILE}
            ${SHADER_DEST_DIR}/${SHADER_FILENAME}
            COPYONLY
    )
    message(STATUS "Configured copy for: ${SHADER_FILENAME} -> ${SHADER_DEST_DIR}")
endforeach ()

# --- Link Dependencies ---
target_include_directories(VkGameProjectOne PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VkGameProjectOne PRIVATE
        ${SDL2_LIBRARIES}
        ${Vulkan_LIBRARIES}
        spdlog::spdlog
        glm::glm
)

message(STATUS "VkGameProjectOne setup complete with spdlog and fetched glm. Configure and build.")