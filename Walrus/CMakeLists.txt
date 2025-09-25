# Walrus Library CMakeLists.txt
project(Walrus)

# Define the static library
add_library(${PROJECT_NAME} STATIC
    src/Walrus/Application.cpp
    src/Walrus/Random.cpp
    src/Walrus/EventLoop.cpp
    src/Walrus/Application.h
    src/Walrus/Layer.h
    src/Walrus/EntryPoint.h
    src/Walrus/Random.h
    src/Walrus/Timer.h
    src/Walrus/EventLoop.h
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        src
    PRIVATE
        src/Walrus
)

# Set C++ standard for the target
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Threading library for std::thread
find_package(Threads REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    Threads::Threads
    ftl
)