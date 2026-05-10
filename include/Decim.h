#pragma once
#include <Butterworth.h>

template <uint fs, uint decim>
struct Decim
{
  protected:
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<6>, 2> lowpass_decim;
    int frame = 0;

  public:
    static constexpr uint output_sr = fs / decim;
    Decim() : frame(0)
    {
        static_assert(decim > 0 && fs % decim == 0);
        lowpass_decim.setup(6, fs, 0.5 * fs / decim);
    }
    size_t process(float *real, float *imag, size_t size_in)
    {
        if (size_in == 0)
        {
            return 0;
        }
        float *c[] = {real, imag};
        lowpass_decim.process(size_in, c);
        size_t number_ret = 0;
        for (size_t i = 0; i < size_in; i++) //todo : optimize that
        {
            if (frame == 0)
            {
                real[number_ret] = real[i];
                imag[number_ret] = imag[i];
                number_ret++;
            }
            frame++;
            frame %= decim;
        }
        return number_ret;
    }
};
struct IMultiStageDecim
{
    virtual ~IMultiStageDecim() = default;
    virtual size_t process(float *real, float *imag, size_t size_in) = 0;
};
template <uint fs, uint FirstStage, uint... RestStages>
struct MultiStageDecim : public IMultiStageDecim
{
    Decim<fs, FirstStage> first_stage;
    MultiStageDecim<fs / FirstStage, RestStages...> next_stages;
    MultiStageDecim() : first_stage(), next_stages() {}
    static constexpr uint output_sr = decltype(next_stages)::output_sr / FirstStage;
    size_t process(float *real, float *imag, size_t size_in) override
    {
        size_t n_out = first_stage.process(real, imag, size_in);
        if (n_out == 0)
            return 0;
        return next_stages.process(real, imag, n_out);
    }
};



template <uint fs, uint stage>
struct MultiStageDecim<fs, stage> : public IMultiStageDecim
{
    Decim<fs, stage> decim;
    static constexpr uint output_sr = decltype(decim)::output_sr / stage;
    MultiStageDecim() : decim() {}
    size_t process(float *real, float *imag, size_t size_in) override
    {
        size_t n_out = decim.process(real, imag, size_in);
        return n_out;
    }
};

typedef MultiStageDecim<48000, 15, 10, 5> decim48k_64;
typedef MultiStageDecim<96000, 15, 10, 10> decim96k_64;
typedef MultiStageDecim<192000, 15, 10, 20> decim192k_64;

static inline std::unique_ptr<IMultiStageDecim> createMultiStageDecim64(int samplerate)
{
    if (samplerate == 48000)
        return std::make_unique<decim48k_64>();
    if (samplerate == 96000)
        return std::make_unique<decim96k_64>();
    if (samplerate == 192000)
        return std::make_unique<decim192k_64>();
    assert(false);
}