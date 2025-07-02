cmake_minimum_required(VERSION 3.16)
project(qtg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-march=native>
)
add_compile_options(
  -march=native
  -mavx2
  -Wextra -Wreorder
)

if (WIN32)
  add_compile_definitions(RPC_NO_WINDOWS_H WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

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
target_include_directories(DSPFilters PUBLIC ${dspfilters_SOURCE_DIR}/DSPFilters/include ${dspfilters_SOURCE_DIR}/include)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

find_package(readerwriterqueue CONFIG REQUIRED)
find_package(kissfft CONFIG REQUIRED)
find_package(RtAudio REQUIRED)
find_package(imgui REQUIRED)
find_package(implot REQUIRED)

find_package(CLI11 REQUIRED)

add_executable(qtg
    src/main.cpp
    src/ResultViewer.cpp
    src/SoundCardDrift.cpp
    src/QuartzDSP.cpp
)

target_include_directories(qtg PRIVATE src ${concurrentqueue_SOURCE_DIR})
find_package(OpenGL REQUIRED)

#target_compile_definitions(qtg PRIVATE IMGUI_DEFINE_MATH_OPERATORS)


target_link_libraries(qtg PRIVATE glfw OpenGL::GL imgui::imgui implot::implot RtAudio::rtaudio DSPFilters  readerwriterqueue::readerwriterqueue )
target_link_libraries(qtg PRIVATE kissfft::kissfft-float CLI11::CLI11)