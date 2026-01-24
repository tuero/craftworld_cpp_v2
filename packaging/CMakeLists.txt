include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (NOT DEFINED craftworld_INSTALL_CMAKEDIR)
   set(craftworld_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/craftworld"
       CACHE STRING "Path to craftworld CMake files")
endif ()

install(TARGETS craftworld EXPORT craftworld_Targets
        RUNTIME COMPONENT craftworld_Runtime
        LIBRARY COMPONENT craftworld_Runtime
        NAMELINK_COMPONENT craftworld_Development
        ARCHIVE COMPONENT craftworld_Development
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(DIRECTORY "${craftworld_SOURCE_DIR}/include/" 
        TYPE INCLUDE
        COMPONENT craftworld_Development)

if (BUILD_SHARED_LIBS)
    set(type shared)
    message(STATUS "Building craftworld shared lib")
else ()
    set(type static)
    message(STATUS "Building craftworld static lib")
endif ()

install(EXPORT craftworld_Targets
        DESTINATION "${craftworld_INSTALL_CMAKEDIR}"
        NAMESPACE craftworld::
        FILE craftworld-${type}-targets.cmake
        COMPONENT craftworld_Development)

write_basic_package_version_file(
    craftworldConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/craftworldConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/craftworldConfigVersion.cmake"
        DESTINATION "${craftworld_INSTALL_CMAKEDIR}"
        COMPONENT craftworld_Development)
