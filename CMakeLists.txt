cmake_minimum_required(VERSION 3.16)
project(qtg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# Options communes pour tous les builds (à adapter)
add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-ffast-math>
    -Wextra -Wreorder
)

# Options spécifiques pour le build natif uniquement
if (NOT CMAKE_CXX_COMPILER MATCHES "em\\+\\+")
    add_compile_options(
      $<$<CONFIG:Release>:-march=native>
      -march=native
      -mavx2
    )
endif()


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

find_package(imgui  CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)

find_package(CLI11 REQUIRED)

set(QTG_SOURCES
    src/ResultViewer.cpp
    src/SoundCardDrift.cpp
    src/QuartzDSP.cpp
)


find_package(OpenGL REQUIRED)
message(STATUS "IMGUI include dirs: ${imgui_INCLUDE_DIRS}")
#target_compile_definitions(qtg PRIVATE IMGUI_DEFINE_MATH_OPERATORS)


#target_link_libraries(qtg PRIVATE glfw OpenGL::GL imgui::imgui implot::implot RtAudio::rtaudio DSPFilters  readerwriterqueue::readerwriterqueue )
#target_link_libraries(qtg PRIVATE kissfft::kissfft-float CLI11::CLI11)

#message(STATUS "CMAKE=_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
add_compile_options(-D_LIBCPP_ENABLE_CXX20_REMOVED_FEATURES)
# === Native build ===
if(NOT ${VCPKG_TARGET_TRIPLET} STREQUAL "wasm32-emscripten")
    find_package(RtAudio REQUIRED)

    add_executable(qtg
        src/main.cpp
        ${QTG_SOURCES}
    )

    target_include_directories(qtg PRIVATE ${QTG_INCLUDES})
    target_link_libraries(qtg PRIVATE
        glfw OpenGL::GL
        imgui::imgui implot::implot
        RtAudio::rtaudio
        DSPFilters
        readerwriterqueue::readerwriterqueue
        kissfft::kissfft-float
        CLI11::CLI11
    )

# === WebAssembly build ===
else()


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sAUDIO_WORKLET -sWASM_WORKERS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
    add_executable(qtg_wasm
        src/main_wasm.cpp
        #${QTG_SOURCES}
    )

    target_include_directories(qtg_wasm PRIVATE ${QTG_INCLUDES})

    target_link_libraries(qtg_wasm PRIVATE
        glfw
        imgui::imgui implot::implot
        DSPFilters
        readerwriterqueue::readerwriterqueue
        kissfft::kissfft-float
    )

    set_target_properties(qtg_wasm PROPERTIES SUFFIX ".html")  # to produce .html+js+wasm
endif()