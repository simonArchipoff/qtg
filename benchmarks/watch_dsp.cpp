#include <catch2/catch_all.hpp>
#include <vector>
#include <algorithm>
#include "../tests/SimWatchSignal.hpp"

#include <FrequencyCounter.h>

TEST_CASE("Benchmark WATCH") {
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
    uint idx =0;
    BENCHMARK("RT") {
        for(int i = 0; i < 10*sr/bs; i++){
            std::vector<float> tmp = sig.getSignal(bs);
            dsp.rt.rt_process(tmp);
            idx += bs;
            dsp.runAsync();
        }
    };
#if 1
    BENCHMARK("results"){
        Result r;
        if(dsp.getResult(r)){
            auto f = r.strongest_frequency();
            double diff = abs(f-actual_frequency);
        } else{
        }
    };
#endif
}


