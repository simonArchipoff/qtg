#include <FrequencyCounter.h>



void FrequencyCounter_rt::rt_process(std::vector<float> &input_block)
{
    auto input_block_data = input_block.data();
    auto input_size = input_block.size();

    if(config.hilbert_shape_preprocessing){
        hilbert.process(input_size,input_block_data,tmp_buff_i.data(),tmp_buff_q.data());
        for(uint i = 0; i < input_size; i++){
            input_block_data[i] = std::hypotf(tmp_buff_i[i], tmp_buff_q[i]);
        }
    }

    bandpass.process(input_size, &input_block_data);
    for (size_t i = 0; i < input_size; ++i)
    {
        auto f = frame++ % config.sample_rate;

        auto t = static_cast<double>(f) / static_cast<double>(config.sample_rate);
        auto phase = -2 * M_PI * config.lo_freq * t;
        double c, s;
        sincos(phase, &s, &c);

        tmp_buff_i[i] = c * input_block_data[i];
        tmp_buff_q[i] = s * input_block_data[i];
    }
    float *c[]= {tmp_buff_i.data(),tmp_buff_q.data()};
    lowpass.process(input_size, c);

    for (unsigned int i = 0; i < input_size; i++)
    {
        if (phase_decim % config.decimation_factor == 0)
        {
            auto out = std::complex<float>(tmp_buff_i[i], tmp_buff_q[i]);
            outputQueue.enqueue(out);
        }
        phase_decim++; // todo optimize that
    }
}