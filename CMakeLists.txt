cmake_minimum_required(VERSION 3.15...3.30)

project(MyProject VERSION 1.0
                  DESCRIPTION "Kero Game Framework Example"
                  LANGUAGES C)
if(APPLE)
    enable_language(OBJC)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "")
    message(WARNING "CMAKE_BUILD_TYPE not specified. Defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(APPLE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Creating Mac OS bundle")
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

add_library(resources STATIC source/resources/resources.c)
add_library(update STATIC source/framework/update.c)
target_link_libraries(update resources)
add_library(osinterface STATIC source/framework/osinterface.c)
add_library(sound STATIC source/framework/sound.c)
add_library(framework STATIC source/framework/framework.c)

add_library(game STATIC source/game/game.c)
add_library(gameplay STATIC source/game/gameplay.c)
add_library(game_menu STATIC source/game/game_menu.c)

target_link_libraries(a update game gameplay game_menu resources osinterface sound framework)
# target_link_options(a PRIVATE -static -static-libgcc)
# Not setting -static because I need to be selective in static vs dynamic linking

if(WIN32)
    target_link_options(a PRIVATE -mwindows -pthread)
    target_link_libraries(a ntdll glew32s glu32 opengl32 avrt ksuser)
    add_compile_definitions(GLEW_STATIC)
    include_directories(../libraries/glew-2.1.0/include)
    target_link_directories(a PRIVATE ../libraries/glew-2.1.0/lib/Release/x64)
elseif(LINUX)
    add_compile_definitions(GLEW_STATIC)
    target_link_libraries(a X11 GLEW GL GLU m Xfixes Xrandr pulse pulse-simple)
elseif(APPLE)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    set_source_files_properties(source/framework/osinterface.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(osinterface GLEW::GLEW "-framework Cocoa -framework CoreVideo -framework OpenGL")
    set_source_files_properties(source/framework/sound.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(sound "-framework AudioUnit")
endif(WIN32)