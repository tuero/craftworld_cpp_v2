cmake_minimum_required (VERSION 3.23)

project(craftworld VERSION 1.0.4)

option(BUILD_PYTHON_MODULE "Build the python library wrapper" OFF)

# Let craftworld_SHARED_LIBS override BUILD_SHARED_LIBS
if (DEFINED craftworld_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${craftworld_SHARED_LIBS}")
endif ()


# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Sources
set(CRAFTWORLD_SOURCES
    include/craftworld/craftworld.h
    include/craftworld/craftworld_base.h
    include/craftworld/definitions.h
    src/craftworld_base.cpp 
)

# CPP library
add_library(craftworld)
add_library(craftworld::craftworld ALIAS craftworld)
set_target_properties(craftworld PROPERTIES
                      VERSION ${craftworld_VERSION}
                      SOVERSION ${craftworld_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
)
target_include_directories(
    craftworld PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_features(craftworld PUBLIC cxx_std_20)
target_sources(craftworld PRIVATE ${CRAFTWORLD_SOURCES})


# Include the install rules if the user wanted them (included by default when top-level)
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
option(craftworld_INCLUDE_PACKAGING "Include packaging rules for craftworld" "${is_top_level}")
if(craftworld_INCLUDE_PACKAGING)
    add_subdirectory(packaging)
endif()


# Pybind11
if(BUILD_PYTHON_MODULE)
    include(FetchContent)
    message("Configuring Pybind11")
    FetchContent_Declare(pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG master
        SYSTEM
    )
    FetchContent_MakeAvailable(pybind11)

    # Python module
    pybind11_add_module(pycraftworld python/pycraftworld.cpp)
    target_link_libraries(pycraftworld PRIVATE craftworld)
    install(TARGETS pycraftworld DESTINATION .)
endif()


# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
