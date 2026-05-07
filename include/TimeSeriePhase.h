#pragma once
#include <complex>
#include <vector>
#include <cassert>
#include <cstdint>
#include <numeric> // std::lcm
#include <deque>


struct TimeSeriePhase
{
    std::deque<size_t> frames;
    std::vector<std::deque<float>> phases;
    uint max_size = 64;

    void addSample(size_t frame, const std::vector<std::complex<float>> &states)
    {
        if (phases.size() == 0)
        {
            phases.resize(states.size());
        }
        else
        {
            assert(phases.size() == states.size());
        }
        frames.push_back(frame);
        bool full = frames.size() > max_size;
        if (full)
            frames.pop_front();
        for (uint i = 0; i < states.size(); i++)
        {
            phases[i].push_back(std::arg(states[i]));
            if (full)
                phases[i].pop_front();
        }
    }

    std::vector<uint64_t> getTimeStamp() const{
        std::vector<uint64_t> res(frames.begin(),frames.end());
        return res;
    }

    std::vector<float> getUnwrappedSerie(int idx) const{
        assert(phases[idx].size() > 1);
        std::vector<float> res(phases[idx].begin(), phases[idx].end());

        const float TWO_PI = 2.0f * M_PI;
        int k = 0;

        for (size_t i = 1; i < res.size(); i++){
            float diff = (res[i] + k*TWO_PI) - res[i - 1];

            if (diff > M_PI)
                k -= 1;
            else if (diff < -M_PI)
                k += 1;

            res[i] += k * TWO_PI;
        }

        return res;
    }
};