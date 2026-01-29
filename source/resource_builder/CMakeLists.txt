cmake_minimum_required(VERSION 3.15...3.30)
project(MyProject VERSION 1.0
                  DESCRIPTION "My cool project"
                  LANGUAGES C)
                  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE DEBUG)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH MY_PARENT_DIR)
string(LENGTH "${MY_PARENT_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DCMAKE_SOURCE_BASE_DIRECTORY_LENGTH=${SOURCE_PATH_SIZE}")

include_directories(../framework)

add_library(folders STATIC ../framework/folders.c ../framework/log.c)
target_compile_definitions(folders PRIVATE FOLDERS_PRINT_ERRORS)

add_executable(resource_builder source/main.c)
target_include_directories(resource_builder PRIVATE "source")
target_compile_options(resource_builder PRIVATE -Wall -Werror -Wno-initializer-overrides -Wno-format-security)
target_link_libraries(resource_builder PRIVATE folders)