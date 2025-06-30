#include <CLI/CLI.hpp>
#include <RtAudio.h>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include "QuartzDSP.h"
#include "ResultViewer.h"

#include "AudioSourceThread.h"

#include "CircularBuffer.h"
#include "Constants.h"


int main(int argc, char **argv)
{
    CLI::App app{"qtg, a quartz watch timegrapher"};

    bool list_devices = false;
    app.add_flag("-l,--list", list_devices, "List available input devices");

    int selected_device_index = -1;
    std::vector<std::string> inputDevices;

    app.add_option("-d,--device", selected_device_index, "Select input device by index");
    
    uint sampleRate = 96000;
    app.add_option("-s,--sample-rate",sampleRate,"requested sampleRate");

    double compensation = 0.0;
    app.add_option("-c,--compensation",compensation, "sound card compensation");

    CLI11_PARSE(app, argc, argv);


    if(list_devices){
        auto devices = RtAudioCaptureThread::listInputDevices();
        for(auto & i : devices){
            std::cout << i << std::endl;
        }
        exit(0);
    }

    QuartzDSPConfig c;
    QuartzDSP dsp(c);
    RtAudioCaptureThread input(dsp,selected_device_index,1024*64);

    
    assert(input.getSampleRate() == 192000 || input.getSampleRate() == 96000);

    ResultViewer viewer;
    viewer.onReset = [&dsp](){dsp.reset();};
    viewer.onApplyCorrection =[&dsp](double v){dsp.setCompensation(v);};
    input.start();
    while (!viewer.shouldClose())
    {
        bool new_buffer = false;

        DriftResult d;
        while(input.getDriftSoundcard(d)){ 
            viewer.pushDriftResult(d);
        }
        std::vector<std::complex<float>> tmp;
        dsp.async.circbuf.get_ordered(tmp,128);
        viewer.pushRawData(tmp);
        struct Result r;
        if(dsp.getResult(r))
            viewer.pushResult(r);
        if(  input.soundcarddrift.execute() && input.soundcarddrift.getSize() > 3)
        {   
            auto r = input.soundcarddrift.getResult(input.soundcarddrift.getSize(), CLOCK_TAI);
            viewer.pushDriftResult(r);
        }
        viewer.renderFrame();
    }
    input.stop();
    return 0;
}
