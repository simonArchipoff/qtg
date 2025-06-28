#include <CLI/CLI.hpp>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>
#include "ResultViewer.h"

#include "AudioSourceThread.h"
#include <kiss_fft.h>
#include "CircularBuffer.h"
#include "Constants.h"




void compute_fft_complex(const std::vector<std::complex<float>> &time_data,
                         std::vector<std::complex<float>> &freq_data)
{
    size_t nfft = time_data.size();
    assert(freq_data.size() == nfft);

    kiss_fft_cfg cfg = kiss_fft_alloc(static_cast<int>(nfft), 0, nullptr, nullptr);
    if (!cfg)
        throw std::runtime_error("kiss_fft_alloc failed");

    std::vector<kiss_fft_cpx> in(nfft);
    std::vector<kiss_fft_cpx> out(nfft);

    for (size_t i = 0; i < nfft; ++i)
    {
        float w = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (nfft - 1)));
        in[i].r = time_data[i].real() * w;
        in[i].i = time_data[i].imag() * w;
    }

    kiss_fft(cfg, in.data(), out.data());

    for (size_t i = 0; i < nfft; ++i)
    {
        freq_data[i] = std::complex<float>(out[i].r, out[i].i);
    }

    free(cfg);
}

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

    using RACT = RtAudioCaptureThread;

    if(list_devices){
        auto devices = RACT::listInputDevices();
        for(auto & i : devices){
            std::cout << i << std::endl;
        }
        exit(0);
    }

    CircularBuffer<std::complex<float>> circbuf(1024*32);

    RACT input(selected_device_index,sampleRate);

    
    assert(input.getSampleRate() == 192000 || input.getSampleRate() == 96000);

    ResultViewer viewer;
    viewer.onReset = [&circbuf](){circbuf.reset();};
    viewer.onApplyCorrection =[&compensation](double v){compensation = v;};
    input.start();
    bool c;
    while (!viewer.shouldClose())
    {
        bool new_buffer = false;
        RACT::BufferPtr b;
        while (input.getBuffer(b))
        {
            new_buffer = true;
            circbuf.push_back(*b);
            viewer.pushRawData(*b);
            input.returnBuffer(b);
        }
        DriftResult d;
        while(input.getDriftSoundcard(d)){ 
            viewer.pushDriftResult(d);
        }

        if (new_buffer)
        {
            std::vector<complex<float>> tmp;
            circbuf.get_ordered(tmp);
            std::vector<complex<float>> out;
            out.resize(tmp.size());
            compute_fft_complex(tmp, out);

            struct Result r;

            uint sr = input.getSampleRate() / input.getDecimationFactor();
            r.time = 1.0 * tmp.size() / sr;
            r.progress = 1.0 * circbuf.size() / circbuf.capacity();
            float f0 = sr / static_cast<float>(out.size());
            for (uint i = 0; i < out.size() / 2; i++)
            {
                auto tmp = i * f0 + 32760;
                if (abs(tmp-Constants::QUARTZ_FREQUENCY) < 8)
                {
                    r.frequencies.push_back(tmp);
                    float energy_pos = std::norm(out[i]);
                    r.magnitudes.push_back(energy_pos);
                }
            }
                if (r.frequencies.size() > 2){
                    r.correction_spm = compensation;
                    viewer.pushResult(r);

                }
            }
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
