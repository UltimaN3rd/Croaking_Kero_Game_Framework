cmake_minimum_required(VERSION 3.15...3.30)

project(MyProject VERSION 1.0
                  DESCRIPTION "Heli"
                  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "")
    message(WARNING "CMAKE_BUILD_TYPE not specified. Defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(APPLE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Creating Mac OS bundle")
    add_executable(a MACOSX_BUNDLE source/framework/main.c)
else()
    add_executable(a source/framework/main.c)
endif()

# Recommended build types: Debug for general debugging, RelWithDebInfo for testing optimized version, MinSizeRel for actual release.
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(a PRIVATE -static-libgcc)
    if(WIN32)
        target_link_options(a PRIVATE -static)
    endif()
endif()
add_compile_options(-Wall -Werror -Wno-initializer-overrides -Wno-format-security -Wno-initializer-overrides -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)
if(NOT APPLE)
    add_compile_options(-fmax-errors=5)
endif()

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(a PRIVATE -static)
endif()

add_compile_definitions(OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE OSINTERFACE_FRAME_BUFFER_SCALED OSINTERFACE_COLOR_INDEX_MODE)
include_directories("source/game" "source/framework" "source/resources")

add_subdirectory("source/framework")

add_library(resources STATIC source/resources/resources.c)

add_library(game STATIC source/game/game.c)
add_library(gameplay STATIC source/game/gameplay.c)
add_library(game_menu STATIC source/game/game_menu.c)

if(NOT APPLE)
    target_link_libraries(a "$<LINK_GROUP:RESCAN,game,gameplay,game_menu,framework,resources>")
else()
    target_link_libraries(a game gameplay game_menu resources framework)
endif()

if(WIN32)
    target_link_options(a PRIVATE -mwindows -pthread)
elseif(APPLE)
    target_link_libraries(a "-framework Cocoa -framework CoreVideo -framework OpenGL -framework AudioUnit")
endif(WIN32)