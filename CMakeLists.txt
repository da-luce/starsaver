cmake_minimum_required(VERSION 3.10)
project(starsaver C)

# Set C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set build type to Debug Optimized
set(CMAKE_BUILD_TYPE DebugOptimized)

# Enable compiler warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -D_XOPEN_SOURCE_EXTENDED -g")

# Add directories for includes
include_directories(include)
include_directories(data)

# Set PKG_CONFIG_PATH to Homebrew's pkgconfig directory (for M1 Macs)
set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/lib/pkgconfig") # for M1 Macs

# set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig")  # for Intel Macs

# Use pkg-config to get the flags and libraries for ncursesw
execute_process(
    COMMAND pkg-config --cflags --libs ncursesw
    RESULT_VARIABLE result
    OUTPUT_VARIABLE NCURSES_FLAGS
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Define the source and header files
set(PROJECT_SOURCE_FILES
    src/astro.c
    src/bit.c
    src/coord.c
    src/core.c
    src/core_position.c
    src/core_render.c
    src/drawing.c
    src/main.c
    src/parse_BSC5.c
    src/stopwatch.c
    src/term.c
    data/keplerian_elements.c
)

set(PROJECT_HEADER_FILES
    include/astro.h
    include/bit.h
    include/coord.h
    include/core.h
    include/core_position.h
    include/core_render.h
    include/drawing.h
    include/parse_BSC5.h
    include/stopwatch.h
    include/term.h
    data/keplerian_elements.h
)

# Create the executable
add_executable(starsaver ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})

if(result EQUAL 0)
    message(STATUS "Found ncursesw using pkg-config: ${NCURSES_FLAGS}")

    # Split the output into include and library flags
    string(REPLACE "-I" " -I" NCURSES_INCLUDE_DIRS "${NCURSES_FLAGS}")
    string(REPLACE "-L" " -L" NCURSES_LIBRARY_DIRS "${NCURSES_FLAGS}")

    # Define the necessary preprocessor macros for ncursesw
    add_definitions(-DHAVE_NCURSESW)

    # Add the include directories and link directories
    include_directories(${NCURSES_INCLUDE_DIRS})
    link_directories(${NCURSES_LIBRARY_DIRS})

    # Link the ncurses library to the target
    target_link_libraries(starsaver ${NCURSES_FLAGS})
else()
    message(WARNING "Failed to find ncursesw using pkg-config. Falling back to regular ncurses.")
    find_package(Curses REQUIRED)
    include_directories(${CURSES_INCLUDE_DIR})
    target_link_libraries(starsaver ${CURSES_LIBRARIES})
endif()
