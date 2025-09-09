cmake_minimum_required(VERSION 3.20)
project(wmspal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Static linking option
option(STATIC_LINKING "Enable static linking" OFF)
option(MUSL_STATIC "Enable musl static linking" OFF)

if(STATIC_LINKING OR MUSL_STATIC)
    # Force static linking for all libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    message(STATUS "Static linking enabled")
endif()

if(MUSL_STATIC)
    set(CMAKE_C_COMPILER "musl-gcc")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    message(STATUS "Using musl-gcc for static compilation")
endif()

# Try to find curl with pkg-config first, then fallback to system
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    if(STATIC_LINKING OR MUSL_STATIC)
        # For static linking, we need to use special flags
        set(PKG_CONFIG_EXECUTABLE "pkg-config")
        pkg_check_modules(CURL REQUIRED libcurl)
        pkg_check_modules(GEOS QUIET geos)
        pkg_check_modules(PROJ QUIET proj)
        # Override with static flags
        if(CURL_FOUND)
            execute_process(COMMAND pkg-config --libs --static libcurl OUTPUT_VARIABLE CURL_STATIC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(CURL_LIBRARIES ${CURL_STATIC_LIBS})
        endif()
        if(GEOS_FOUND)
            execute_process(COMMAND pkg-config --libs --static geos OUTPUT_VARIABLE GEOS_STATIC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(GEOS_LIBRARIES ${GEOS_STATIC_LIBS})
        endif()
        if(PROJ_FOUND)
            execute_process(COMMAND pkg-config --libs --static proj OUTPUT_VARIABLE PROJ_STATIC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
            set(PROJ_LIBRARIES ${PROJ_STATIC_LIBS})
        endif()
    else()
        pkg_check_modules(CURL REQUIRED libcurl)
        pkg_check_modules(GEOS QUIET geos)
        pkg_check_modules(PROJ QUIET proj)
    endif()
else()
    find_package(CURL REQUIRED)
endif()

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
if(PkgConfig_FOUND AND CURL_FOUND)
    target_link_libraries(wmspal ${CURL_LIBRARIES})
    target_include_directories(wmspal PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_options(wmspal PRIVATE ${CURL_CFLAGS_OTHER})
else()
    target_link_libraries(wmspal CURL::libcurl)
endif()

# Link GEOS and PROJ if available
if(GEOS_FOUND)
    target_link_libraries(wmspal ${GEOS_LIBRARIES})
    target_include_directories(wmspal PRIVATE ${GEOS_INCLUDE_DIRS})
    target_compile_definitions(wmspal PRIVATE HAVE_GEOS)
    message(STATUS "Building with GEOS support")
endif()

if(PROJ_FOUND)
    target_link_libraries(wmspal ${PROJ_LIBRARIES})
    target_include_directories(wmspal PRIVATE ${PROJ_INCLUDE_DIRS})
    target_compile_definitions(wmspal PRIVATE HAVE_PROJ)
    message(STATUS "Building with PROJ support")
endif()

if(WIN32)
    target_link_libraries(wmspal ws2_32)
endif()