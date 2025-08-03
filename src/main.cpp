#include <CLI/CLI.hpp>
#include <RtAudio.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>
#include "CLI/CLI.hpp"
#include "QuartzDSP.h"
#include "ResultViewer.h"

#include "AudioSourceThread.h"

#include "CircularBuffer.h"




int main(int argc, char *argv[])
{
    CLI::App app{"qtg, a quartz watch timegrapher"};
    QuartzDSPConfig c;
    bool list_devices = false;
    app.add_flag("-l,--list", list_devices, "List available input devices");

    int selected_device_index = -1;
    std::vector<std::string> inputDevices;

    app.add_option("-d,--device", selected_device_index, "Select input device by index");
    
    app.add_option("-s,--sample-rate",c.sample_rate,"Requested sampleRate");

    app.add_option("-f,--frequency",c.target_freq,"Nominal frequency");
    app.add_option("--decimation",c.decimation_factor,"Decimation factor (this can be very high, 32000 by default)");
    app.add_option("--local-oscillator",c.lo_freq,"Local oscillator frequency (by default the same as frequency)");
    app.add_option("--bw-bandpass",c.bw_bandpass,"Width arround the frequency to select");
    app.add_option("--integration-time",c.duration_analysis_s, "Integration time for a measure");
    enum Unit unit = Unit::Second_Per_Month;
    std::map<std::string, Unit> map{
            {"hertz",            Unit::Hertz},
            {"s/day",            Unit::Second_Per_Day},
            {"s/month",          Unit::Second_Per_Month},
            {"s/year",           Unit::Second_Per_Year},
            {"ppm",              Unit::PPM}};
    app.add_option("--unit", unit, "Unit of the result")
        ->transform(CLI::CheckedTransformer(map,CLI::ignore_case).description(CLI::detail::generate_map(CLI::detail::smart_deref(map), true)));

    CLI11_PARSE(app, argc, argv);
    if(app.get_option("--local-oscillator")->count() == 0){
        c.lo_freq = c.target_freq;
    }

    if(c.target_freq > (c.sample_rate / 2.0)){
        throw std::runtime_error("samplerate to low for the target frequency");
    }
    if(c.lo_freq > (c.sample_rate / 2.0)){
        throw std::runtime_error("samplerate to low for the lo frequency");
    }
    if((c.target_freq - c.lo_freq) > ((double) c.sample_rate / c.decimation_factor) / 2.0 ){
        throw std::runtime_error("Decimation to high for the difference between the frequency and the local oscillator frequency");
    }
    if(c.bw_bandpass > (c.target_freq - c.lo_freq)){
        std::cerr << "maybe the bandpass width is too wide and we'll have some aliasing, but let's run anyway";
    }

    if(list_devices){
        auto devices = RtAudioCaptureThread::listInputDevices();
        for(auto & i : devices){
            std::cout << i << std::endl;
        }
        exit(0);
    }

    QuartzDSP dsp(c);
    RtAudioCaptureThread input(dsp,selected_device_index,1024*16);

    
    assert(input.getSampleRate() == 192000 || input.getSampleRate() == 96000);

    ResultViewer viewer(unit);
    viewer.onReset = [&dsp](){dsp.reset();};
    viewer.onApplyCorrection =[&dsp](double v){dsp.setRealSR(v);};
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
        if(input.soundcarddrift.execute() && input.soundcarddrift.getSize() > 3)
        {   
            auto r = input.soundcarddrift.getResult(input.soundcarddrift.getSize());
            viewer.pushDriftResult(r);
        }
        viewer.vumeter.push_level(input.get_level());
        viewer.renderFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    input.stop();
    return 0;
}
