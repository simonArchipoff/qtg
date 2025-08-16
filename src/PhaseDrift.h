#pragma once

#if 0
#include "CircularBuffer.h"
#include <complex>
#include <vector>
#include <cstddef>
#include <complex.h>
#include <readerwriterqueue.h>


class PhaseDrift_rt{
    public:
    const std::size_t sr,duration_sec;
    const int frequency;

    std::size_t frame = 0;
    OnePole output;
    moodycamel::ReaderWriterQueue<std::complex<double>> outputQueue;
    PhaseDrift_rt(size_t sr
                 , int frequency
                 , double tau_filter):
        sr(sr)
        ,frequency(frequency)
        ,frame(0)
        ,output(1.0/tau_filter, sr)
    {
    }

    void rt_process(const std::vector<float> & input_block){
        for (size_t i = 0; i < input_block.size(); ++i){ 
            auto t = static_cast<double>(frame++) / static_cast<double>(sr);
            auto phase = 2 * M_PI * frequency * t;
            double c, s;
            sincos(phase, &s, &c);
            std::complex<double> lo(c,s);
            output.process(lo * std::complex<double>(static_cast<double>(input_block[i]),0.0));
            if(frame % sr == 0){
                frame = 0;
                outputQueue.try_enqueue(output.getState());
            }
        }
    }

    bool getPhase(std::complex<double> & r){
        return outputQueue.try_dequeue(r);
    }
};

class PhaseDrift{
    public:
    PhaseDrift_rt rt;
    std::optional<double> last_angle;
    PhaseDrift(size_t sr,int frequency, double tau_filter):
        rt(sr,frequency,tau_filter)
        {
        }

    void runAsync(){
        std::complex<double> p;
        while(rt.getPhase(p)){
            auto a = std::arg(p);
            if(last_angle){
                auto new_angle = a -  last_angle.value();
                b.push_back(new_angle);
            }
            last_angle = a;
        }
    }
    double getFrequency(){
        return 0.0;
    }
};
#endif