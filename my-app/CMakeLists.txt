cmake_minimum_required(VERSION 3.20.0)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stm32f723_app)

# ==========================================
# Force CMake Re-run
# ==========================================
# 1. Automate .c files (Sources)
# ==========================================
file(GLOB_RECURSE APP_SOURCES 
    "src/*.c"
    "drivers/*.c"
)
target_sources(app PRIVATE ${APP_SOURCES})
# ==========================================
# 2. Automate .h files (Include Directories)
# ==========================================
file(GLOB_RECURSE APP_HEADERS 
    "src/*.h"
    "drivers/*.h"
)
# Extract just the folder path from each header file
set(APP_INCLUDE_DIRS "")
foreach(HEADER_FILE ${APP_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    list(APPEND APP_INCLUDE_DIRS ${HEADER_DIR})
endforeach()
# Remove duplicate folders from the list
list(REMOVE_DUPLICATES APP_INCLUDE_DIRS)
# Give the list of folders to the compiler
target_include_directories(app PRIVATE ${APP_INCLUDE_DIRS})