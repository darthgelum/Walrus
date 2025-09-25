cmake_minimum_required(VERSION 3.15)

project(WalrusApp VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Walrus Framework Configuration Options
option(WALRUS_ENABLE_EVENT_LOOP "Enable EventLoop functionality" ON)
option(WALRUS_ENABLE_PUBSUB "Enable PubSub functionality" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# Platform defines
if(WIN32)
    add_compile_definitions(WL_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(WL_PLATFORM_LINUX)
elseif(APPLE)
    add_compile_definitions(WL_PLATFORM_MACOS)
endif()

# Build type defines
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(WL_DEBUG)
else()
    add_compile_definitions(WL_RELEASE)
endif()

# Walrus Framework defines
if(WALRUS_ENABLE_EVENT_LOOP)
    add_compile_definitions(WALRUS_ENABLE_EVENT_LOOP=1)
else()
    add_compile_definitions(WALRUS_ENABLE_EVENT_LOOP=0)
endif()

if(WALRUS_ENABLE_PUBSUB)
    add_compile_definitions(WALRUS_ENABLE_PUBSUB=1)
else()
    add_compile_definitions(WALRUS_ENABLE_PUBSUB=0)
endif()

# Add subdirectories
add_subdirectory(Walrus)
add_subdirectory(WalrusApp)