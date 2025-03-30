cmake_minimum_required(VERSION 3.30.5)

project(VkProjectOne LANGUAGES CXX)

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
FetchContent_MakeAvailable(glm)

# --- Download stb_image.h using file(DOWNLOAD) ---
set(STB_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/_deps/stb-download)
set(STB_IMAGE_HEADER ${STB_DOWNLOAD_DIR}/stb_image.h)
set(STB_IMAGE_URL "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h")

file(MAKE_DIRECTORY ${STB_DOWNLOAD_DIR})

if(NOT EXISTS ${STB_IMAGE_HEADER})
    message(STATUS "Downloading stb_image.h from ${STB_IMAGE_URL}")
    file(DOWNLOAD
            ${STB_IMAGE_URL}
            ${STB_IMAGE_HEADER}
            SHOW_PROGRESS
            STATUS download_status
            TIMEOUT 60
    )
    list(GET download_status 0 download_error_code)
    list(GET download_status 1 download_error_string)
    if(NOT download_error_code EQUAL 0)
        message(FATAL_ERROR "Failed to download stb_image.h: ${download_error_string}")
    endif()
    message(STATUS "Downloaded stb_image.h successfully.")
else()
    message(STATUS "stb_image.h already exists. Skipping download.")
endif()

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
        core/TerrainLoader.cpp
        core/TerrainLoader.h
        core/Terrain.cpp
        core/Terrain.h
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

# --- Copy Assets Directory ---
set(SOURCE_ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
set(DEST_ASSET_DIR ${CMAKE_BINARY_DIR}/assets)

if(EXISTS ${SOURCE_ASSET_DIR})
    message(STATUS "Source assets directory found: ${SOURCE_ASSET_DIR}")
    file(COPY
            ${SOURCE_ASSET_DIR}/ # Note the trailing slash: copies CONTENTS
            DESTINATION ${DEST_ASSET_DIR} # Copies into this directory
    )
    message(STATUS "Copied assets from ${SOURCE_ASSET_DIR}/ to ${DEST_ASSET_DIR}")
else()
    message(WARNING "Source assets directory not found: ${SOURCE_ASSET_DIR} - Cannot copy assets.")
endif()

# --- Link Dependencies ---
target_include_directories(VkGameProjectOne PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
        ${STB_DOWNLOAD_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VkGameProjectOne PRIVATE
        ${SDL2_LIBRARIES}
        ${Vulkan_LIBRARIES}
        spdlog::spdlog
        glm::glm
)

message(STATUS "VkGameProjectOne setup complete with spdlog and fetched glm. Configure and build.")