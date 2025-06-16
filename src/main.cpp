
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <iomanip> // pour std::setprecision
#include <CorrelationBank.h>
#include "AudioSourceFlac.h"
#include "ResultViewer.h"
#include <random>
double random_double() {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> dist(-1.0, 1.0);
    return dist(rng);
}
int main(int ac, char ** av){
    
    auto devices = RtAudioCaptureThread::listInputDevices();
    RtAudioCaptureThread input(131);
    
    std::vector<double> freqs ;
    for(int i = -80; i < 80; i+=1){
        double n = 100;
        freqs.push_back(32768+i/n);
    }

    CorrelationBank bank;
    assert(input.getSampleRate() == 192000);
    bank.init(input.getSampleRate()+0.35,freqs);
    

    
    ResultViewer viewer;
    viewer.onReset = [&bank](){bank.reset();};
    std::cerr<< "start\n";
    input.start();
    std::cerr << "after start\n";
    bool c;
    while (!viewer.shouldClose()) {
        bool new_buffer=false;
        RtAudioCaptureThread::BufferPtr b;
        while(input.getBuffer(b))
        {
            new_buffer=true;
            bank.processBlock(*b);
            input.returnBuffer(b);
        }
        if(new_buffer){
            viewer.pushResult(bank.getResult());
        }

        viewer.renderFrame();
    }
    input.stop();
    return 0;
}
