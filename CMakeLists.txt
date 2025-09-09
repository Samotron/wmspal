cmake_minimum_required(VERSION 3.20)
project(wmspal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Static linking option
option(STATIC_LINKING "Enable static linking" OFF)

if(STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(UNIX AND NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
    message(STATUS "Static linking enabled")
endif()

# Find required packages using vcpkg
find_package(CURL REQUIRED)
find_package(geos QUIET)
find_package(proj4 QUIET)

include_directories(include)

set(SOURCES
    src/main.c
    src/wms.c
    src/georeference.c
    src/vectorize.c
    src/attribution.c
)

add_executable(wmspal ${SOURCES})

# Link curl (required)
target_link_libraries(wmspal CURL::libcurl)

# Link GEOS and PROJ if available
if(geos_FOUND)
    target_link_libraries(wmspal geos::geos)
    target_compile_definitions(wmspal PRIVATE HAVE_GEOS)
    message(STATUS "Building with GEOS support")
endif()

if(proj4_FOUND)
    target_link_libraries(wmspal proj4::proj4)
    target_compile_definitions(wmspal PRIVATE HAVE_PROJ)
    message(STATUS "Building with PROJ support")
endif()

if(WIN32)
    target_link_libraries(wmspal ws2_32)
endif()