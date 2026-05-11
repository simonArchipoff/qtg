#include "SimWatchSignal.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <FrequencyCounter.h>

#include <npy/npy.h>



TEST_CASE("FrequencyCounter DSP w", "[Watch]") {
    FrequencyCounterConfig c;
    const uint sr = 48000;
    c.sample_rate_nominal = sr;
    c.lo_freq = 0;
    c.analytic_signal = true;
    c.duration_analysis_s = 4;
    c.harmonics = 1;
    c.duration_between_analysis = 0.2;
    //c.decimation_factor = 48000/64;

    uint total_time = sr * 100;

    FrequencyCounterDSP dsp(c);

    const uint bs = 16;
    int base_frequency = 4;
    WatchSimSignal sig(sr, base_frequency);
    double ppm = 100;
    sig.setDrift(ppm);
    std::vector<std::complex<float>> res;
    double actual_frequency = static_cast<double>(base_frequency) * (1 + (ppm / 1e6));
    dsp.rt.init(bs);

    SECTION("Valeurs par défaut") {
        uint idx =0;
        while(idx + bs < total_time){
            std::vector<float> tmp = sig.getSignal(bs);
            
            //for(uint i = 0; i < bs; i++)
            //    res[ idx + i] = tmp[i];
            dsp.rt.rt_process(tmp);
            idx += bs;
            //dsp.runAsync();
            std::complex<float> o;
            //while(dsp.rt.getOut(o)){
            //    res.push_back(o);
            //}

        }
        Result r;
        if(dsp.getResult(r)){
            auto f = r.strongest_frequency();
            double diff = abs(f-actual_frequency);
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
        t.save("dump_out_watch_dsp.npy");
#endif
    }
}
