#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <FrequencyCounter.h>

#ifdef QTG_WITH_NPY
#include <npy/npy.h>
#endif

#if 1
TEST_CASE("FrequencyCounter DSP", "[FrequencyCounter]") {
    FrequencyCounterConfig c;
    const uint sr = 48000;
    c.sample_rate_nominal = sr;
    c.lo_freq = 4;
    c.duration_analysis_s = 4;
    c.duration_between_analysis = 0.2;
    //c.decimation_factor = 48000/64;

    FrequencyCounterDSP dsp(c);

    const uint bs = 16;
    double frequency = 6-3.12345678;

    std::vector<float> signal;
    signal.resize(35 * sr);
    for(uint i = 0; i < signal.size(); i++){
        signal[i] = sin(static_cast<double>(i) * frequency * 2 * M_PI / static_cast<double>(sr));
    }
    dsp.rt.init(bs);
    std::vector<std::complex<float>> res;
    res.reserve(signal.size());

    SECTION("Valeurs par défaut") {
        uint idx =0;
        while(idx + bs < signal.size()){
            std::vector<float> tmp(signal.data() + idx,
                                   signal.data() + idx + bs);
            dsp.rt.rt_process(tmp);
            idx += bs;

            dsp.runAsync();
        }

        Result r;
        if(dsp.getResult(r)){
            auto f = r.strongest_frequency();
            double diff = abs(f-frequency);
            REQUIRE(diff < 0.0001);
        } else{
            REQUIRE(false);
        }
#if 0
        std::vector<size_t> shape({res.size(),2});
        npy::tensor<float> t(shape);
        for(uint i = 0; i < res.size(); i++){
            t(i,0)=res[i].real();
            t(i,1)=res[i].imag();
        }
        t.save("dump_out_dsp.npy");
#endif
    }
}
#endif
