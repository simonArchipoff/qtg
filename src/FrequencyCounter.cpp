#include <FrequencyCounter.h>

FrequencyCounter_rt::FrequencyCounter_rt(FrequencyCounterConfig &c) : config(c)
{
    frame = 0;
    phase_decim = 0;
    assert(config.lo_freq * 2 <= config.sample_rate_nominal);
    auto sample_rate = config.sample_rate_nominal;
    auto decimation_factor = config.decimation_factor;

    auto fc = sample_rate / (2.0 * decimation_factor);
    lowpass.setup(4,sample_rate,fc);
}

void FrequencyCounter_rt::rt_process(std::vector<float> &input_block)
{
    auto input_block_data = input_block.data();
    auto input_size = input_block.size();

    for (size_t i = 0; i < input_size; ++i)
    {
        auto t = static_cast<double>(frame) / static_cast<double>(config.sample_rate_nominal);
        frame = (frame + 1); // % config.sample_rate;
        auto phase = -2 * M_PI * config.lo_freq * t;
        double c, s;
        sincos(phase, &s, &c);

        tmp_buff_i[i] = c * input_block_data[i];
        tmp_buff_q[i] = s * input_block_data[i];
    }

    float *c[] = {tmp_buff_i.data(), tmp_buff_q.data()};
    lowpass.process(input_size, c);

    for (unsigned int i = 0; i < input_size; i++)
    {
        float *tmp[] = {&tmp_buff_i[i], &tmp_buff_q[i]};
        if (phase_decim % config.decimation_factor == 0)
        {
            auto out = std::complex<float>(tmp_buff_i[i], tmp_buff_q[i]);
            outputQueue.enqueue(out);
        }
        phase_decim++; // todo optimize that
    }
}

bool FrequencyCounterDSPAsync::getResult(Result &r)
{
    if (!freq_measurement.history_size())
        return false;
    r.magnitudes = freq_measurement.getMagnitude();
    r.frequencies = freq_measurement.getFrequencies(this->real_sr);
    for (auto &f : r.frequencies)
    {
        f += config.lo_freq;
    }
    auto snr = freq_measurement.getSNR();

    r.frequencies_corrected_phasedrift = r.frequencies;
    auto imax = std::distance(snr.begin(), std::max_element(snr.begin(), snr.end()));

    if (freq_measurement.history_size() > 1)
    {
        for (uint i = 0; i < r.frequencies.size(); i++)
        {
            std::vector<size_t> t;
            std::vector<float> p;
            freq_measurement.getPhases(i, t, p);
            auto ri = ::getPhaseDriftResult(this->real_sr, t, p);
            r.frequencies_corrected_phasedrift[i] += ri.frequency;
        }
    }
    return true;
}

FrequencyCounterDSPAsync::FrequencyCounterDSPAsync(FrequencyCounterConfig &c)
    : config(c), real_sr(static_cast<double>(c.sample_rate_nominal) / c.decimation_factor)
{
    auto d_sr = c.sample_rate_nominal / c.decimation_factor; // init value, not actual sr
    auto s = kiss_fft_next_fast_size(d_sr * c.duration_analysis_s);
    freq_measurement.init(s, std::max(1, static_cast<int>(d_sr * c.duration_between_analysis)));
}
