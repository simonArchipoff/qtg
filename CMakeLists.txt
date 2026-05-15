cmake_minimum_required(VERSION 3.16)
project(qtg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

# ============================================================
# Options
# ============================================================
option(QTG_ENABLE_DEV "Enable development tools (tests, benchmarks, npy)" OFF)

# ============================================================
# Compile options (globales)
# ============================================================
add_compile_options(
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-fno-fast-math>
    -Wextra -Wreorder
    $<$<CONFIG:Profile>:-O2>
    $<$<CONFIG:Profile>:-g>
    $<$<CONFIG:Profile>:-pg>
)

if(WIN32)
    add_compile_definitions(RPC_NO_WINDOWS_H WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_compile_options(-D_LIBCPP_ENABLE_CXX20_REMOVED_FEATURES)

# ============================================================
# DSPFilters — FetchContent (dépendance externe)
# ============================================================
include(FetchContent)
FetchContent_Declare(
    DSPFilters
    GIT_REPOSITORY https://github.com/jcelerier/DSPFilters.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(DSPFilters)
file(GLOB_RECURSE DSPFILTERS_HEADER_FILES
    "${dspfilters_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE DSPFILTERS_SOURCE_FILES
    "${dspfilters_SOURCE_DIR}/src/*.cpp")

add_library(DSPFilters STATIC
    ${DSPFILTERS_SOURCE_FILES}
    ${DSPFILTERS_HEADER_FILES}
)
target_include_directories(DSPFilters PUBLIC
    ${dspfilters_SOURCE_DIR}/DSPFilters/include
    ${dspfilters_SOURCE_DIR}/include
)

# ============================================================
# Dépendances vcpkg
# ============================================================
find_package(readerwriterqueue CONFIG REQUIRED)
find_package(kissfft CONFIG REQUIRED)
if(QTG_ENABLE_DEV)
    find_package(npy REQUIRED)
endif()
find_package(CLI11 REQUIRED)
find_package(RtAudio REQUIRED)

# Catch2 — optionnel (tests/benchmarks)
if(QTG_ENABLE_DEV)
    find_package(Catch2 REQUIRED)
endif()

# ============================================================
# Tests (CTest)
# ============================================================
include(CTest)
enable_testing()

# ============================================================
# Sous-projets
# ============================================================
add_subdirectory(src)

# Tests
if(QTG_ENABLE_DEV AND Catch2_FOUND)
    add_subdirectory(tests)
    add_test(NAME qtg_tests COMMAND qtg_tests)
    set_tests_properties(qtg_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Benchmarks
if(QTG_ENABLE_DEV AND Catch2_FOUND)
    add_subdirectory(benchmarks)
endif()
