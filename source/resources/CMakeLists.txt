cmake_minimum_required(VERSION 3.15...3.30)
project(MyProject VERSION 1.0
                  DESCRIPTION "My cool project"
                  LANGUAGES C)
                  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE DEBUG)

include_directories(../framework)

add_library(folders STATIC ../framework/folders.c ../framework/log.c)
target_compile_definitions(folders PRIVATE FOLDERS_PRINT_ERRORS)

add_executable(a source/main.c)
target_include_directories(a PRIVATE "source")
target_compile_options(a PRIVATE -Wall -Werror -Wno-initializer-overrides -Wno-format-security)
target_link_libraries(a PRIVATE folders)