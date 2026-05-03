#pragma once
#include <cstddef>
#include <vector>

class DSPModule_rt{
    public:
    virtual ~DSPModule_rt() = default;
    virtual size_t sampleRateNominal() const = 0;
    virtual void init(std::size_t input_size) = 0;
    virtual void rt_process(std::vector<float> &input_block) = 0;
};
