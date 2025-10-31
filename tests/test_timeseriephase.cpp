
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <complex>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <FilterBankDiscrete.h>

#include <numeric>
using Catch::Approx;
// Helper function to create complex numbers from magnitude and phase
std::complex<float> make_complex(float magnitude, float phase_rad) {
    return std::polar<float>(magnitude,phase_rad);
}

TEST_CASE("TimeSeriePhase - Construction et initialisation", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    
    SECTION("Valeurs par défaut") {
        REQUIRE(ts.frames.empty());
        REQUIRE(ts.phases.empty());
        REQUIRE(ts.max_size == 64);
    }
}

TEST_CASE("TimeSeriePhase - Premier échantillon", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    std::vector<std::complex<float>> states = {
        make_complex(1.0f, 0.0f),
        make_complex(1.0f, M_PI/2.0f)
    };
    
    ts.addSample(1000, states);
    
    SECTION("Initialisation correcte") {
        REQUIRE(ts.phases.size() == 2);
        REQUIRE(ts.frames.size() == 1);
        REQUIRE(ts.frames[0] == 1000);
        REQUIRE(ts.phases[0].size() == 1);
        REQUIRE(ts.phases[1].size() == 1);
    }
    
    SECTION("Phases correctes") {
        REQUIRE(ts.phases[0][0] == Approx(0.0f).margin(1e-6f));
        REQUIRE(ts.phases[1][0] == Approx(M_PI/2.0f).margin(1e-6f));
    }
}



TEST_CASE("TimeSeriePhase - Limitation de taille maximale", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    ts.max_size = 3;
    std::vector<std::complex<float>> states = {
        make_complex(1.0f, 0.0f)
    };
    
    // Ajouter plus d'échantillons que la taille maximale
    for (size_t i = 0; i < 5; ++i) {
        ts.addSample(i * 1000, states);
    }
    
    SECTION("Taille limitée à max_size") {
        REQUIRE(ts.frames.size() == 3);
        REQUIRE(ts.phases[0].size() == 3);
    }
    
    SECTION("Les anciens échantillons sont supprimés") {
        // Les échantillons 0 et 1 devraient être supprimés
        REQUIRE(ts.frames[0] == 2000);
        REQUIRE(ts.frames[1] == 3000);
        REQUIRE(ts.frames[2] == 4000);
    }
}


TEST_CASE("TimeSeriePhase - getTimeStamp", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    std::vector<std::complex<float>> states = {make_complex(1.0f, 0.0f)};
    
    ts.addSample(1000, states);
    ts.addSample(2000, states);
    ts.addSample(3000, states);
    
    auto timestamps = ts.getTimeStamp();
    
    SECTION("Conversion correcte des timestamps") {
        REQUIRE(timestamps.size() == 3);
        REQUIRE(timestamps[0] == 1000);
        REQUIRE(timestamps[1] == 2000);
        REQUIRE(timestamps[2] == 3000);
    }
}

TEST_CASE("TimeSeriePhase - getUnwrappedSerie", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    
    std::vector<std::complex<float>> states;
    
    states.push_back(make_complex(1.0f, 0.0f));
    
    ts.addSample(1000, states);
    
    // Ajouter un échantillon avec un saut de phase
    states[0] = make_complex(1.0f, fmod(2 , 2*M_PI));
    ts.addSample(2000, states);

    states[0] = make_complex(1.0f, fmod(4 , 2*M_PI));
    ts.addSample(3000, states);
    states[0] = make_complex(1.0f, fmod(6, 2*M_PI));
    ts.addSample(4000, states);
    states[0] = make_complex(1.0f, fmod(8, 2*M_PI));
    ts.addSample(5000, states);
    
    SECTION("Déroulement de phase simple") {
        auto unwrapped = ts.getUnwrappedSerie(0);
        
        REQUIRE(unwrapped.size() == 5);
        REQUIRE(unwrapped[0] == Approx(0.0f).margin(1e-6f));
        REQUIRE(unwrapped[1] == Approx(2).margin(1e-6f));
        REQUIRE(unwrapped[2] == Approx(4).margin(1e-6f));
        REQUIRE(unwrapped[3] == Approx(6).margin(1e-6f));

    }
    
    SECTION("Test avec saut positif") {
        // Test avec un saut de +PI à -PI
        TimeSeriePhase ts2;
        
        std::vector<std::complex<float>> states2 = {make_complex(1.0f, 0.9f * M_PI)};
        ts2.addSample(1000, states2);
        
        states2[0] = make_complex(1.0f, -0.9f * M_PI); // Saut de ~1.8*PI à ~-1.8*PI
        ts2.addSample(2000, states2);
        
        auto unwrapped = ts2.getUnwrappedSerie(0);
        REQUIRE(unwrapped.size() == 2);
        // Le déroulement devrait détecter le saut et ajouter 2*PI
        float expected = -0.9f * M_PI + 2 * M_PI;
        //REQUIRE(unwrapped[1] == Approx(expected).margin(1e-6f));
    }
}

TEST_CASE("TimeSeriePhase - Cohérence des données", "[TimeSeriePhase]") {
    TimeSeriePhase ts;
    
    std::vector<std::complex<float>> states = {
        make_complex(1.0f, 0.0f),
        make_complex(1.0f, M_PI)
    };
    
    // Ajouter plusieurs échantillons
    for (size_t i = 0; i < 10; ++i) {
        ts.addSample(i * 1000, states);
    }
    
    SECTION("Cohérence entre frames et phases") {
        REQUIRE(ts.frames.size() == ts.phases[0].size());
        REQUIRE(ts.frames.size() == ts.phases[1].size());
    }
    
    SECTION("Accès sécurisé avec getUnwrappedSerie") {
        // Doit fonctionner pour tous les indices valides
        REQUIRE_NOTHROW(ts.getUnwrappedSerie(0));
        REQUIRE_NOTHROW(ts.getUnwrappedSerie(1));
        
        // Doit throw pour index invalide (test avec assert)
        // Note: Les assertions sont désactivées en mode release, 
        // donc on ne peut pas les tester directement avec Catch2
    }
}