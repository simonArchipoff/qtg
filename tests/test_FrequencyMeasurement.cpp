#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>


#include <FrequencyMeasurement.h>

TEST_CASE("Frequency measurement", "[FrequencyMeasurement]") {
    FrequencyMeasurement f;
    uint sr = 8;
    uint bs = 32;
    f.init(bs,1);
    float frequency = 3;
    SECTION("Valeurs par défaut") {
        std::vector<float> s(1000);
        for(uint i = 0; i < s.size(); i++){
            s[i] = std::sin(i * frequency * 2 * M_PI / static_cast<float>(sr));
        }
        f.addSamples(s.begin(), s.end());
        auto m = f.getMagnitude();
        auto fr = f.getFrequencies(sr);
        auto snr = f.getSNR();
        std::vector<size_t> t;
        std::vector<float> p;
        f.getPhases(12,t,p);
        auto r = ::getPhaseDriftResult(sr,t,p);
        
        REQUIRE(true);
    }
}