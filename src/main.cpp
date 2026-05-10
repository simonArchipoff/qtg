#include <CLI/CLI.hpp>
#include <RtAudio.h>
#include <iostream>
#include <thread>

#include <vector>
#include <string>
#include "CLI/CLI.hpp"

#include "AudioSourceThread.h"

#include <CircularBuffer.h>

#include <ResultSignal.h>
#include <FrequencyCounter.h>

int main(int argc, char *argv[])
{
    CLI::App app{"qtg, a quartz watch timegrapher"};
    FrequencyCounterConfig c;
    bool list_devices = false;
    app.add_flag("-l,--list", list_devices, "List available input devices");

    int selected_device_index = -1;
    std::vector<std::string> inputDevices;

    app.add_option("-d,--device", selected_device_index, "Select input device by index");

    app.add_option("-s,--sample-rate", c.sample_rate_nominal, "Requested sampleRate");

    app.add_option(
        "--decimation", c.decimation_factor, "Decimation factor (this can be very high)");
    app.add_option("--local-oscillator", c.lo_freq,
        "Local oscillator frequency (by default the same as frequency)");
    //app.add_option("--bw-bandpass",c.bw_bandpass,"Width arround the frequency to select");
    app.add_option("--integration-time", c.duration_analysis_s, "Integration time for a measure");
    enum Unit unit = Unit::Second_Per_Month;
    std::map<std::string, Unit> map{{"hertz", Unit::Hertz}, {"s/day", Unit::Second_Per_Day},
        {"s/month", Unit::Second_Per_Month}, {"s/year", Unit::Second_Per_Year}, {"ppm", Unit::PPM}};
    app.add_option("--unit", unit, "Unit of the result")
        ->transform(CLI::CheckedTransformer(map, CLI::ignore_case)
                .description(CLI::detail::generate_map(CLI::detail::smart_deref(map), true)));

    CLI11_PARSE(app, argc, argv);

    if (list_devices)
    {
        auto devices = RtAudioCaptureThread::listInputDevices();
        for (auto &i : devices)
        {
            std::cout << i << std::endl;
        }
        exit(0);
    }

    FrequencyCounterDSP dsp(c);
    RtAudioCaptureThread input(dsp.rt, selected_device_index, 1024 * 16);

    assert(input.getSampleRate() == 192000 || input.getSampleRate() == 96000);

    input.start();
    while (true)
    {
        bool new_buffer = false;
        DriftResult d;

        std::vector<std::complex<float>> tmp;
        struct Result result;
        if (dsp.getResult(result))
        {
        }
        if (input.soundcarddrift.execute() && input.soundcarddrift.getSize() > 3)
        {
            auto r = input.soundcarddrift.getResult(input.soundcarddrift.getSize());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    input.stop();
    return 0;
}
