#pragma once
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
    std::complex<double> currentPhase;
    moodycamel::ReaderWriterQueue<std::complex<double>> outputQueue;
    PhaseDrift_rt(size_t sr,int frequency, size_t duration_sec):
        sr(sr)
        ,duration_sec(duration_sec)
        ,frequency(frequency)
        ,frame(0)
        ,currentPhase(0.0,0.0)
    {
    }

    void rt_process(std::vector<float> &input_block){
        for (size_t i = 0; i < input_block.size(); ++i){ 
            auto t = static_cast<double>(frame++) / static_cast<double>(sr);
            auto phase = 2 * M_PI * frequency * t;
            double c, s;
            sincos(phase, &s, &c);
            std::complex<double> lo(c,s);
            currentPhase += lo * std::complex<double>(input_block[i],0.0);

            if(frame % (duration_sec * sr) == 0){
                frame = 0;// I think its ok to do that
                outputQueue.try_enqueue(currentPhase);

                currentPhase.imag(0.0);
                currentPhase.real(0.0);
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
    CircularBuffer<double> b;
    std::optional<double> last_angle;
    PhaseDrift(size_t sr,int frequency, size_t duration_sec,size_t integration_time_sec):
        rt(sr,frequency,duration_sec)
        ,b(integration_time_sec / duration_sec)
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
        
    }


};