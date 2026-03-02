cmake_minimum_required(VERSION 3.15...3.30)

project(MyProject VERSION 1.0
                  DESCRIPTION "Croaking Kero Game Framework"
                  LANGUAGES C)
if(APPLE)
    enable_language(OBJC)
endif()
if(WIN32)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Check for correct compiler versions
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_C_COMPILER_VERSION VERSION_LESS 22)
        message (FATAL_ERROR "Your Clang compiler version ${CMAKE_C_COMPILER_VERSION} is too old. Requires either Clang 22+ or GCC 15+")
    else ()
        add_compile_options(-fdefer-ts)
    endif ()
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_C_COMPILER_VERSION VERSION_LESS 15)
        message (FATAL_ERROR "Your GCC compiler version ${CMAKE_C_COMPILER_VERSION} is too old. Requires either Clang 22+ or GCC 15+")
    endif ()
else ()
    message (WARNING "Unknown compiler - if you are unable to build, please try using Clang 22+ or GCC 15+")
endif ()

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not specified. Defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Recommended build types: Debug for general debugging, RelWithDebInfo for testing optimized version, MinSizeRel for actual release.
add_compile_options(-Wall -Werror -Wno-initializer-overrides -Wno-format-security -Wno-initializer-overrides -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)
if(NOT APPLE)
    add_compile_options(-fmax-errors=5)
endif()

add_compile_definitions(OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE OSINTERFACE_FRAME_BUFFER_SCALED OSINTERFACE_COLOR_INDEX_MODE EXPLORER_PRINT_ERRORS FOLDERS_PRINT_ERRORS)

add_library(osinterface OBJECT osinterface.c)
if(WIN32)
    add_library(sound OBJECT sound_windows.cpp sound_windows.c)
else()
    add_library(sound OBJECT sound.c)
endif()
add_library(OpenGL2_1 OBJECT OpenGL2_1.c)

add_library(framework 
    framework.c
    $<TARGET_OBJECTS:osinterface>
    $<TARGET_OBJECTS:sound>
    $<TARGET_OBJECTS:OpenGL2_1>)

target_link_options(framework PRIVATE -Wl,--whole-archive)

if(WIN32)
    target_link_options(framework PRIVATE -mwindows -pthread)
    target_link_libraries(framework PRIVATE ntdll glu32 opengl32 avrt ksuser stdc++)
elseif(LINUX)
    target_link_libraries(framework PRIVATE X11 GL GLU m Xfixes Xrandr pulse pulse-simple)
elseif(APPLE)
    set_source_files_properties(osinterface.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(osinterface "-framework Cocoa -framework CoreVideo -framework OpenGL")
    set_source_files_properties(OpenGL2_1.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(OpenGL2_1 "-framework Cocoa -framework CoreVideo -framework OpenGL")
    set_source_files_properties(sound.c PROPERTIES LANGUAGE OBJC)
    target_link_libraries(sound "-framework AudioUnit")
    set_source_files_properties(framework.c PROPERTIES LANGUAGE C) # This line probably unnecessary - remember to remove next time working on Mac
endif(WIN32)