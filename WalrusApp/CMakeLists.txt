# WalrusApp CMakeLists.txt
project(WalrusApp)

# Define the executable
add_executable(${PROJECT_NAME}
    src/WalrusApp.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ../Walrus/src
)

# Link with Walrus library
target_link_libraries(${PROJECT_NAME} Walrus)

# Set C++ standard for the target
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)