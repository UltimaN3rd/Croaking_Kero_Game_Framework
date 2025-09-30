cmake_minimum_required(VERSION 3.15...3.30)

project(MyProject VERSION 1.0
                  DESCRIPTION "Croaking Kero Game Framework"
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

# Recommended build types: Debug for general debugging, RelWithDebInfo for testing optimized version, MinSizeRel for actual release.
add_compile_options(-Wall -Werror -Wno-initializer-overrides -Wno-format-security -Wno-initializer-overrides -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)
if(NOT APPLE)
    add_compile_options(-fmax-errors=5)
endif()

add_compile_definitions(OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE OSINTERFACE_FRAME_BUFFER_SCALED OSINTERFACE_COLOR_INDEX_MODE EXPLORER_PRINT_ERRORS FOLDERS_PRINT_ERRORS)
include_directories("../game" "../resources")

add_library(cereal OBJECT cereal.c)
add_library(explorer OBJECT explorer.c)
add_library(folders OBJECT folders.c)
add_library(log OBJECT log.c)
add_library(menu OBJECT menu.c)
add_library(utilities OBJECT utilities.c)
add_library(render OBJECT render.c)
add_library(samples OBJECT samples.c)
add_library(sprite OBJECT sprite.c)
add_library(update OBJECT update.c)
add_library(resources OBJECT ../resources/resources.c)
add_library(osinterface OBJECT osinterface.c)
add_library(sound OBJECT sound.c)
# add_library(internal_framework OBJECT framework.c)

add_library(framework STATIC $<TARGET_OBJECTS:cereal> $<TARGET_OBJECTS:explorer> $<TARGET_OBJECTS:folders> $<TARGET_OBJECTS:log> $<TARGET_OBJECTS:menu> $<TARGET_OBJECTS:utilities> $<TARGET_OBJECTS:render> $<TARGET_OBJECTS:samples> $<TARGET_OBJECTS:sprite> $<TARGET_OBJECTS:update> $<TARGET_OBJECTS:resources> $<TARGET_OBJECTS:osinterface> $<TARGET_OBJECTS:sound>)# $<TARGET_OBJECTS:internal_framework>)

target_link_options(framework PRIVATE -Wl,--whole-archive)

# Later remove GLEW dependency

if(WIN32)
    target_link_options(framework PRIVATE -mwindows -pthread)
    target_link_libraries(framework PRIVATE ntdll glew32s glu32 opengl32 avrt ksuser)
    include_directories(../../libraries/glew-2.1.0/include)
    target_link_directories(framework PRIVATE ../../libraries/glew-2.1.0/lib/Release/x64)
elseif(LINUX)
    find_package(GLEW REQUIRED)
    target_link_libraries(framework PRIVATE X11 GLEW::GLEW GL GLU m Xfixes Xrandr pulse pulse-simple)
elseif(APPLE)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    set_source_files_properties(osinterface.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(osinterface GLEW::GLEW "-framework Cocoa -framework CoreVideo -framework OpenGL")
    set_source_files_properties(sound.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(sound "-framework AudioUnit")
endif(WIN32)