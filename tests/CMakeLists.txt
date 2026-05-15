# ============================================================
# qtg — tests unitaires
# ============================================================

# quick_gen_data — outil de génération de données (nécessite npy)
if(QTG_ENABLE_DEV)
    add_executable(quick_gen_data test_discrete_bank.cpp)
    target_include_directories(quick_gen_data PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(quick_gen_data PRIVATE npy::npy)
endif()

# --- tests unitaires (nécessitent Catch2) ---
if(QTG_ENABLE_DEV AND Catch2_FOUND)
    add_executable(qtg_tests
        test_timeseriephase.cpp
        test_FrequencyMeasurement.cpp
        test_DSP_frequencyCounter.cpp
        test_hilbert_watch_signal.cpp
    )
    target_include_directories(qtg_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(qtg_tests PRIVATE
        Catch2::Catch2WithMain
        qtg
        npy::npy
    )
endif()
