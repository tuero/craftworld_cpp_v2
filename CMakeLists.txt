cmake_minimum_required (VERSION 3.12)

project(craftworld)

include(FetchContent)

# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Pybind11
message("Configuring Pybind11")
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG master
    SYSTEM
)
FetchContent_MakeAvailable(pybind11)

# Sources
set(CRAFTWORLD_SOURCES
    src/definitions.h
    src/craftworld_base.cpp 
    src/craftworld_base.h 
)

# CPP library
add_library(craftworld STATIC ${CRAFTWORLD_SOURCES})
target_compile_features(craftworld PUBLIC cxx_std_20)
target_include_directories(craftworld PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Python module
pybind11_add_module(pycraftworld EXCLUDE_FROM_ALL python/pycraftworld.cpp)
target_link_libraries(pycraftworld PRIVATE craftworld)
install(TARGETS pycraftworld DESTINATION .)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
