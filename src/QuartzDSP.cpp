
#include "QuartzDSP.h"
#include <kiss_fft.h>


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

bool QuartzDSPAsync::getResult(Result & r){
    if(circbuf.size() < 1){
        return false;
    }
    Result tmp_r;
    r = tmp_r;
    std::vector<complex<float>> tmp;
    circbuf.get_ordered(tmp);
    std::vector<complex<float>> out;
    out.resize(tmp.size());
    compute_fft_complex(tmp, out);

    unsigned int sr = config.sample_rate / config.decimation_factor;
    r.time = 1.0 * tmp.size() / sr;
    r.progress = 1.0 * circbuf.size() / circbuf.capacity();
    float f0 = sr / static_cast<float>(out.size());
    for (unsigned int i = 0; i < out.size() / 2; i++)
    {
        auto tmp = i * f0 + config.lo_freq;
        if (abs(tmp-config.target_freq) < config.bw_bandpass)
        {
            r.frequencies.push_back(tmp);
            float energy_pos = std::norm(out[i]);
            r.magnitudes.push_back(energy_pos);
        }
    }
    return true;
}
void QuartzDSP_rt::rt_process(vector<float> &input_block)
{
    auto tmp = input_block.data();
    auto input_size = input_block.size();
    bandpass.process(input_size, &tmp);
    for (size_t i = 0; i < input_size; ++i)
    {
        auto f = frame++ % config.sample_rate;
        ;
        auto t = static_cast<double>(f) / static_cast<double>(config.sample_rate);
        auto phase = -2 * M_PI * config.lo_freq * t;
        double c, s;
        sincos(phase, &s, &c);

        tmp_buff_i[i] = c * input_block[i];
        tmp_buff_q[i] = s * input_block[i];
    }
    float *i_ptrs[1] = {tmp_buff_i.data()};
    float *q_ptrs[1] = {tmp_buff_q.data()};
    lowpass_i.process(input_size, i_ptrs);
    lowpass_q.process(input_size, q_ptrs);

    for (unsigned int i = 0; i < input_size; i++)
    {
        if (phase_decim % config.decimation_factor == 0)
        {
            auto out = std::complex<float>(
                tmp_buff_i[i], tmp_buff_q[i]);
            outputQueue.enqueue(out);
        }
        phase_decim++; // todo optimize that
    }
}
