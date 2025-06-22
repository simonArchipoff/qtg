
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <iomanip> // pour std::setprecision
#include "AudioSourceFlac.h"
#include "ResultViewer.h"
#include <kiss_fft.h>
#include <random>
#include <CircularBuffer.h>
double random_double()
{
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> dist(-1.0, 1.0);
    return dist(rng);
}


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

int main(int ac, char **av)
{
    using RACT = RtAudioCaptureThread<>;
    auto devices = RACT::listInputDevices();
    CircularBuffer<std::complex<float>> circbuf(1024*32);
    CircularBuffer<DriftResult> driftbuf(128);

    RACT input(131);
    /* target_link_libraries(qtg PRIVATE kissfft::kissfft-float)
    std::vector<double> freqs ;
    for(int i = 0; i < 200; i+=1){
        double n = 700;
        freqs.push_back(32768+i/n);
    } */

    // CorrelationBank bank;
    assert(input.getSampleRate() == 192000 || input.getSampleRate() == 96000);
    // bank.init(input.getSampleRate() + 0.55,freqs);

    ResultViewer viewer;
    viewer.onReset = [&circbuf](){circbuf.reset();};
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
            ;
            compute_fft_complex(tmp, out);

            struct Result r;
            uint sr = input.getSampleRate() / input.getDecimationFactor();
            float f0 = sr / static_cast<float>(out.size());
            for (int i = 0; i < out.size() / 2; i++)
            {
                auto tmp = i * f0 + 32760;
                if (true || abs(tmp-32768) < 20)
                {
                    r.frequencies.push_back(tmp);
                    float energy_pos = std::norm(out[i]);
                    r.magnitudes.push_back(energy_pos);
                }
            }
                std::cout << out.size() << " " << f0 << std::endl;
                if (r.frequencies.size() > 2)
                    viewer.pushResult(r);
            }

            viewer.renderFrame();
        }
        input.stop();
        return 0;
    }
