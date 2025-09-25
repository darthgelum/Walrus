#ifndef WALRUS_CONFIG_H
#define WALRUS_CONFIG_H

// =====================================================
// Walrus Framework Configuration
// =====================================================

// Event Loop Configuration
// WALRUS_ENABLE_EVENT_LOOP is controlled by CMake option (-DWALRUS_ENABLE_EVENT_LOOP=ON/OFF)
// Default: ON (enabled) - use cmake -DWALRUS_ENABLE_EVENT_LOOP=OFF to disable
// This allows build-time configuration of EventLoop functionality.
#ifndef WALRUS_ENABLE_EVENT_LOOP
    #error "WALRUS_ENABLE_EVENT_LOOP must be defined by the build system (CMake)"
#endif

// Additional event loop settings (only used when WALRUS_ENABLE_EVENT_LOOP is enabled)
#if WALRUS_ENABLE_EVENT_LOOP
    // Number of worker threads for parallel callback execution
    // Set to 0 to use hardware_concurrency() automatically
    #ifndef WALRUS_EVENT_LOOP_THREAD_COUNT
        #define WALRUS_EVENT_LOOP_THREAD_COUNT 0
    #endif
    
    // Enable debug logging for event loop operations
    #ifndef WALRUS_EVENT_LOOP_DEBUG
        #define WALRUS_EVENT_LOOP_DEBUG 0
    #endif
#endif

// Other framework features can be added here as needed
// Example:
// #ifndef WALRUS_ENABLE_PUBSUB
//     #define WALRUS_ENABLE_PUBSUB 1
// #endif

#endif // WALRUS_CONFIG_H