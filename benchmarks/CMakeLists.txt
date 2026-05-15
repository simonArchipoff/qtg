# ============================================================
# qtg — benchmarks (standalone, no Catch2)
# ============================================================

if(QTG_ENABLE_DEV)
    # Mechanical watch benchmark (4 Hz, LO=0, analytic_signal=true)
    add_executable(mechanical_watch_dsp mechanical_watch_dsp.cpp)
    target_include_directories(mechanical_watch_dsp PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests
    )
    target_link_libraries(mechanical_watch_dsp PRIVATE
        CLI11::CLI11
        DSPFilters
        qtg
        readerwriterqueue::readerwriterqueue
        kissfft::kissfft-float
        RtAudio::rtaudio
    )
    target_link_options(mechanical_watch_dsp PRIVATE $<$<CONFIG:Profile>:-pg>)

    # Quartz watch benchmark (32768 Hz, LO=32768, analytic_signal=false)
    add_executable(quartz_watch_dsp quartz_watch_dsp.cpp)
    target_include_directories(quartz_watch_dsp PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests
    )
    target_link_libraries(quartz_watch_dsp PRIVATE
        CLI11::CLI11
        DSPFilters
        qtg
        readerwriterqueue::readerwriterqueue
        kissfft::kissfft-float
        RtAudio::rtaudio
    )
    target_link_options(quartz_watch_dsp PRIVATE $<$<CONFIG:Profile>:-pg>)
endif()
