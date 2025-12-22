#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <FrequencyMeasurement.h>
#if 0
TEST_CASE("Frequency measurement", "[FrequencyMeasurement]") {
    FrequencyMeasurement f;
    uint sr = 8;
    uint bs = 32;
    f.init(bs,8);
    float frequency = 3.1;
    SECTION("Valeurs par défaut") {
        std::vector<float> s(1000);
        for(uint i = 0; i < s.size(); i++){
            s[i] = std::sin(i * frequency * 2 * M_PI / static_cast<float>(sr));
        }
        f.addSamples(s.begin(), s.end());
        auto m = f.getMagnitude();
        auto fr = f.getFrequencies(sr);
        auto snr = f.getSNR();
        auto imax = std::distance(snr.begin(),std::max_element(snr.begin(), snr.end()));
        std::vector<size_t> t;
        std::vector<float> p;
        f.getPhases(imax,t,p);
        auto rimax = ::getPhaseDriftResult(sr,t,p);
        rimax.frequency += ::getFrequencyNormBin(imax,bs) * sr;
        f.getPhases(imax-1,t,p); 
        auto r11 = ::getPhaseDriftResult(sr,t,p);
        r11.frequency += ::getFrequencyNormBin(imax-1,bs) * sr;
        f.getPhases(imax+1,t,p);
        auto r13 = ::getPhaseDriftResult(sr,t,p);
        r13.frequency += ::getFrequencyNormBin(imax+1,bs) * sr;

        REQUIRE(true);
    }
}
#endif