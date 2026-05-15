# ============================================================
# qtg — benchmarks
# ============================================================

if(QTG_ENABLE_DEV AND Catch2_FOUND)
    add_executable(benchmarks watch_dsp.cpp)
    target_include_directories(benchmarks PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(benchmarks PRIVATE
        Catch2::Catch2WithMain
        DSPFilters
        qtg
        readerwriterqueue::readerwriterqueue
        kissfft::kissfft-float
    )
    target_link_options(benchmarks PRIVATE $<$<CONFIG:Profile>:-pg>)
endif()
