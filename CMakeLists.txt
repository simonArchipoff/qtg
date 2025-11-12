cmake_minimum_required(VERSION 3.16)
project(qtg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

#set(CMAKE_BUILD_TYPE Profile CACHE STRING "Profiling build" FORCE)

# Options communes pour tous les builds (à adapter)
add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-fno-fast-math> #fastmath breaks it, dont know why, let's not use it
    -Wextra -Wreorder
    $<$<CONFIG:Profile>:-O2>
    $<$<CONFIG:Profile>:-g>
    $<$<CONFIG:Profile>:-pg>
)


add_compile_options(
    -mavx2
)


if (WIN32)
  add_compile_definitions(RPC_NO_WINDOWS_H WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

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
target_include_directories(DSPFilters PUBLIC ${dspfilters_SOURCE_DIR}/DSPFilters/include ${dspfilters_SOURCE_DIR}/include)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

find_package(readerwriterqueue CONFIG REQUIRED)
find_package(kissfft CONFIG REQUIRED)
find_package(imgui  CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(CLI11 REQUIRED)

set(QTG_SOURCES
    src/AudioSourceThread.cpp
    src/ResultViewer.cpp
    src/SoundCardDrift.cpp
    src/QuartzDSP.cpp
)


find_package(OpenGL REQUIRED)

add_compile_options(-D_LIBCPP_ENABLE_CXX20_REMOVED_FEATURES)
find_package(RtAudio REQUIRED)

add_library(libqtg ${QTG_SOURCES})
target_include_directories(libqtg PUBLIC include)
target_link_libraries(libqtg PUBLIC RtAudio::rtaudio
    DSPFilters
    readerwriterqueue::readerwriterqueue
    kissfft::kissfft-float)

add_executable(qtg src/main.cpp ${QTG_SOURCES} )
target_include_directories(qtg PRIVATE ${QTG_INCLUDES} include)
target_link_libraries(qtg PRIVATE
    glfw OpenGL::GL
    imgui::imgui implot::implot
    libqtg
    CLI11::CLI11
)


find_package(npy CONFIG REQUIRED)
add_executable(quick_gen_data tests/test_discrete_bank.cpp )
target_include_directories(quick_gen_data PRIVATE include)
target_link_libraries(quick_gen_data PRIVATE npy::npy)


find_package(Catch2 REQUIRED)
add_executable(test
    tests/test_timeseriephase.cpp
    tests/test_FrequencyMeasurement.cpp
)
target_link_libraries(test PUBLIC libqtg)

# Link with our library and Catch2
target_link_libraries(test 
    PRIVATE 
    Catch2::Catch2WithMain
    libqtg
)


add_executable(benchmarks benchmarks/benchmark_filter_bank.cpp src/QuartzDSP.cpp)
target_include_directories(benchmarks PRIVATE include src)
target_link_libraries(benchmarks PRIVATE Catch2::Catch2WithMain DSPFilters     readerwriterqueue::readerwriterqueue     kissfft::kissfft-float)
#target_compile_options(benchmarks PRIVATE )
target_link_options(benchmarks PRIVATE $<$<CONFIG:Profile>:-pg>)
