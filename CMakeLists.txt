cmake_minimum_required(VERSION 3.29)
project(ohnoes C)

set(CMAKE_C_STANDARD 11)
# if release mode, use -O3 and strip the binary for size
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()

# Find ncurses library
find_package(Curses REQUIRED)

# Add executable and link with ncurses
add_executable(ohnoes main.c
        process.h
        process.c
        search.c
        search.h
        display.c
        display.h)
target_link_libraries(ohnoes PRIVATE ${CURSES_LIBRARIES})

# Include the ncurses headers
target_include_directories(ohnoes PRIVATE ${CURSES_INCLUDE_DIRS})
