cmake_minimum_required(VERSION 2.8.6)

# Default build-time configuration options
# Can be modified via CMake GUI or via CMake command line
option(INTEGER_MAP_CACHE_STOMPER_ENABLED "Stomp on memory between operations" OFF)
option(INTEGER_MAP_TWEAK_PRIORITY_AFFINITY "Lock to a single CPU core and increase thread priority" ON)
option(INTEGER_MAP_USE_DLMALLOC "Use DLMalloc instead of the default C runtime platform malloc" ON)
set(INTEGER_MAP_TIMING_METHOD "RDTSC" CACHE STRING "API used to time code")
set(INTEGER_MAP_EXPERIMENT "INSERT" CACHE STRING "What type of experiment to perform")
set(INTEGER_MAP_CONTAINER "TABLE" CACHE STRING "Which container type to test")
set(INTEGER_MAP_KEY_GENERATION "RANDOM_SEQUENCE_OF_UNIQUE" CACHE STRING "Key creation method")
set(INTEGER_MAP_MAX_ADDRESS_BLOCK_SIZE 256 CACHE INTEGER "Maximum difference between generated address values")

# Valid settings for drop-down lists
set_property(CACHE INTEGER_MAP_TIMING_METHOD PROPERTY STRINGS QUERY_PERFORMANCE_COUNTER RDTSC)
set_property(CACHE INTEGER_MAP_EXPERIMENT PROPERTY STRINGS INSERT LOOKUP MEMORY)
set_property(CACHE INTEGER_MAP_CONTAINER PROPERTY STRINGS NONE JUDY TABLE)
set_property(CACHE INTEGER_MAP_KEY_GENERATION PROPERTY STRINGS LINEAR SORTED_ADDRESSES SHUFFLED_ADDRESSES RANDOM_SEQUENCE_OF_UNIQUE)

# Write build-time configuration options to a header file
configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Create project and .exe
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE INTERNAL "limited configs")
project(CompareIntegerMaps)
file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES *.h)
if (INTEGER_MAP_USE_DLMALLOC)
    list(APPEND SRCFILES dlmalloc/malloc.c)
endif()
add_executable(CompareIntegerMaps ${SRCFILES} ${INCFILES} config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include(VisualStudioSettings.cmake)

# If Judy is used, add the library to the project
if (INTEGER_MAP_CONTAINER STREQUAL "JUDY")
    add_definitions(-DJU_WIN)
    add_subdirectory(JudyL)
    include_directories(JudyL)
    target_link_libraries(CompareIntegerMaps JudyL)
endif()    
