
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <iomanip> // pour std::setprecision
#include <CorrelationBank.h>
#include "AudioSourceFlac.h"
#include "ResultViewer.h"
#include <kiss_fft.h>
#include <random>
double random_double()
{
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<double> dist(-1.0, 1.0);
    return dist(rng);
}

template <typename T>
class CircularBuffer
{
public:
    CircularBuffer(size_t capacity)
        : capacity_(capacity), buffer_(capacity), head_(0), size_(0)
    {
    }

    // Ajoute des données à la fin du buffer (concaténation)
    void push_back(const std::vector<T> &data)
    {
        assert(data.size() <= capacity_);
        for (auto &v : data)
        {
            buffer_[head_] = v;
            head_ = (head_ + 1) % capacity_;
            if (size_ < capacity_)
                size_++;
        }
    }

    // Accès en lecture au buffer dans l’ordre chronologique (ancien vers récent)
    // Remplit out avec le contenu actuel, out doit avoir au moins size_ éléments
    void get_ordered(std::vector<T> &out) const
    {
        out.resize(size_);
        size_t start = (head_ + capacity_ - size_) % capacity_;
        for (size_t i = 0; i < size_; ++i)
        {
            out[i] = buffer_[(start + i) % capacity_];
        }
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    size_t head_; // position d'écriture
    size_t size_; // nb éléments valides
};

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
    CircularBuffer<std::complex<float>> circbuf(1024 * 1024);

    RACT input(131);
    /* target_link_libraries(qtg PRIVATE kissfft::kissfft-float)
    std::vector<double> freqs ;
    for(int i = 0; i < 200; i+=1){
        double n = 700;
        freqs.push_back(32768+i/n);
    } */

    // CorrelationBank bank;
    assert(input.getSampleRate() == 192000);
    // bank.init(input.getSampleRate() + 0.55,freqs);

    ResultViewer viewer;
    // viewer.onReset = [&bank](){bank.reset();};
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
            input.returnBuffer(b);
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
                if (32767 < tmp && tmp < 32769)
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
