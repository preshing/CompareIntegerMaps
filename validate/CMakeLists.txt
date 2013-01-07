cmake_minimum_required(VERSION 2.8.6)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE INTERNAL "limited configs")
project(ValidateHashTable)

#-------- Build --------
option(INTEGER_MAP_USE_DLMALLOC "Use DLMalloc instead of the default C runtime platform malloc" ON)
include(../VisualStudioSettings.cmake)
configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SRCFILES test.cpp ../hashtable.cpp)
set(INCFILES ../hashtable.h ../util.h config.h.in)
if (INTEGER_MAP_USE_DLMALLOC)
    list(APPEND SRCFILES ../dlmalloc/malloc.c)
endif()
add_executable(ValidateHashTable ${SRCFILES} ${INCFILES} config.h.in)

#-------- Test --------
enable_testing()
find_package(PythonInterp)
foreach(seed RANGE 1 100)
    add_test(NAME ValidateHashTable_${seed} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND ${PYTHON_EXECUTABLE} test.py $<TARGET_FILE:ValidateHashTable> ${seed})
endforeach()
