cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Geovision VERSION 1.0.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Set up dependencies, resources, and link Geode.
# The project has an explicit install target for the active Geometry Dash
# mods folder below; skip Geode CLI's separate current-profile install.
set(GEODE_DONT_INSTALL_MODS ON CACHE BOOL "Install Geovision to its configured game mods folder")
setup_geode_mod(${PROJECT_NAME})

# `geode build` packages the mod in the build directory. Also copy the package
# into the active Geometry Dash installation after packaging.
set(GEOVISION_GAME_MODS_DIR "C:/Users/58740286/Desktop/Juegos/Geometry Dash 2.2081 - CreceGames/Geometry Dash/geode/mods")
if (NOT IS_DIRECTORY "${GEOVISION_GAME_MODS_DIR}")
    message(FATAL_ERROR "Geometry Dash Geode mods directory was not found: ${GEOVISION_GAME_MODS_DIR}")
endif()

add_custom_target(install_geovision_package ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/piomix.geovision.geode"
        "${GEOVISION_GAME_MODS_DIR}/piomix.geovision.geode"
    DEPENDS ${PROJECT_NAME}_PACKAGE
    VERBATIM
)
