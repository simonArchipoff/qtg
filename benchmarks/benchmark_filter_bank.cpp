#include <catch2/catch_all.hpp>
#include <vector>
#include <algorithm>


#include <QuartzDSP.h>
#include <FilterBankDiscrete.h>

TEST_CASE("Benchmark comparison") {
    QuartzDSPConfig c;
    QuartzDSP qtg(c);

    auto N = 512;
    qtg.rt.init(N);
    std::vector<std::vector<float>> s(N);
    for(int i = 0; i < N; i++){
        s[i].resize(N);
        for(int j = 0 ; j < N; j++){
            s[i][j] = std::rand()/static_cast<float>(RAND_MAX);
        }
    }


    BENCHMARK("qtg") {
        Result r;
        bool b;
        for(int i = 0; i < s.size(); i++){
            qtg.rt.rt_process(s[i]);
            b = qtg.getResult(r);
        }


    };

    auto target_p = ((float)c.target_freq)/c.sample_rate;
    FilterBankDiscrete<16> filter(target_p * (1 - 0.0001), target_p * (1+ 0.0001),16);

    BENCHMARK("filter_bank") {

        for(int i = 0; i < s.size(); i++){
            filter.process(s[i]);
        }


    };
}