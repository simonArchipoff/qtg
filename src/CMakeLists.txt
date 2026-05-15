# ============================================================
# qtg — bibliothèque + application principale
# ============================================================

set(QTG_SOURCES
    AudioSourceThread.cpp
    SoundCardDrift.cpp
    FrequencyCounter.cpp
    WeirdSTFT.cpp
)

# --- bibliothèque qtg ---
add_library(qtg ${QTG_SOURCES})
target_include_directories(qtg PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(qtg PUBLIC
    RtAudio::rtaudio
    DSPFilters
    readerwriterqueue::readerwriterqueue
    kissfft::kissfft-float
)

if(QTG_ENABLE_DEV)
    target_compile_definitions(qtg PUBLIC QTG_WITH_NPY)
    target_link_libraries(qtg PUBLIC npy::npy)
endif()

# --- executable qtg-app ---
add_executable(qtg-app main.cpp ${QTG_SOURCES})
target_include_directories(qtg-app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(qtg-app PRIVATE
    qtg
    CLI11::CLI11
)
