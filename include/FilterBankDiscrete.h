#pragma once
#include <complex>
#include <vector>
#include <cassert>
#include <cstdint>
#include <numeric> // std::lcm
#include <deque>
#include "TimeSeriePhase.h"

using std::complex;
using std::numeric_limits;
using std::vector;

template <int decim, typename T = uint64_t>
struct Period
{
    using STORE_INT = T;
    STORE_INT period;
    using WIDE_TYPE = typename std::conditional_t<(sizeof(STORE_INT) < 8), uint64_t, __uint128_t>;

    template <typename C>
    Period(C f) : period(f * (1 << decim))
    {
        assert(c(f) * (1 << decim) <= c(numeric_limits<decltype(period)>::max()));
        static_assert(decim >= 0 && decim < 8 * sizeof(STORE_INT));
    }
    float getPeriod() const { return static_cast<double>(period) / (1 << decim); }
    STORE_INT getPhase(u_int64_t i, int normFactor) const
    {
        return ((c(i) * (1 << decim)) % period) * normFactor / period;
    }
    double getPhase(u_int64_t i) const
    {
        auto p = ((c(i) * (1 << decim)) % period);
        return p / static_cast<double>(period);
    }
    STORE_INT getPhaseFixed(u_int64_t i) const
    {
        auto tmp = c(i) * (1 << decim);
        return tmp % period;
    }

  private:
    template <typename C>
    static inline WIDE_TYPE c(C i)
    {
        return static_cast<WIDE_TYPE>(i);
    }
};

template <int decim, typename T>
u_int64_t lcmv(vector<Period<decim, T>> &v)
{
    if (v.empty())
        return 0;
    u_int64_t result = v[0].period;
    for (size_t i = 1; i < v.size(); ++i)
    {
        result = std::lcm(result, static_cast<u_int64_t>(v[i].period));
    }
    return result;
};

struct GeneratorFunction
{
    std::complex<float> get(float phase);
};

struct GenerationFunctionSin : GeneratorFunction
{
    std::complex<float> get(float phase)
    {
        float s, c;
        sincosf(phase, &s, &c);
        return std::complex<float>(c, s);
    }
};



struct GenerationFunctionSinLRU
{
    static constexpr size_t TABLE_SIZE = 64;
    
    // Table statique simple
    static std::complex<float> table[TABLE_SIZE];

    std::complex<float> get(float phase)
    {
        // Ramener la phase à un index simple
        int index = int(phase * TABLE_SIZE / (2.0f * M_PI)) & (TABLE_SIZE - 1);
        return table[index];
    }
};

// Définition de la table (pré-calculée)
std::complex<float> GenerationFunctionSinLRU::table[GenerationFunctionSinLRU::TABLE_SIZE] = {
    {1.0f, 0.0f},{0.995184f, 0.0980171f},{0.980785f, 0.19509f},{0.95694f, 0.290285f},
    {0.92388f, 0.382683f},{0.881921f, 0.471397f},{0.83147f, 0.55557f},{0.77301f, 0.634393f},
    {0.707107f, 0.707107f},{0.634393f, 0.77301f},{0.55557f, 0.83147f},{0.471397f, 0.881921f},
    {0.382683f, 0.92388f},{0.290285f, 0.95694f},{0.19509f, 0.980785f},{0.0980171f, 0.995184f},
    {0.0f, 1.0f},{-0.0980171f, 0.995184f},{-0.19509f, 0.980785f},{-0.290285f, 0.95694f},
    {-0.382683f, 0.92388f},{-0.471397f, 0.881921f},{-0.55557f, 0.83147f},{-0.634393f, 0.77301f},
    {-0.707107f, 0.707107f},{-0.77301f, 0.634393f},{-0.83147f, 0.55557f},{-0.881921f, 0.471397f},
    {-0.92388f, 0.382683f},{-0.95694f, 0.290285f},{-0.980785f, 0.19509f},{-0.995184f, 0.0980171f},
    {-1.0f, 0.0f},{-0.995184f, -0.0980171f},{-0.980785f, -0.19509f},{-0.95694f, -0.290285f},
    {-0.92388f, -0.382683f},{-0.881921f, -0.471397f},{-0.83147f, -0.55557f},{-0.77301f, -0.634393f},
    {-0.707107f, -0.707107f},{-0.634393f, -0.77301f},{-0.55557f, -0.83147f},{-0.471397f, -0.881921f},
    {-0.382683f, -0.92388f},{-0.290285f, -0.95694f},{-0.19509f, -0.980785f},{-0.0980171f, -0.995184f},
    {0.0f, -1.0f},{0.0980171f, -0.995184f},{0.19509f, -0.980785f},{0.290285f, -0.95694f},
    {0.382683f, -0.92388f},{0.471397f, -0.881921f},{0.55557f, -0.83147f},{0.634393f, -0.77301f},
    {0.707107f, -0.707107f},{0.77301f, -0.634393f},{0.83147f, -0.55557f},{0.881921f, -0.471397f},
    {0.92388f, -0.382683f},{0.95694f, -0.290285f},{0.980785f, -0.19509f},{0.995184f, -0.0980171f}
};


template <int decim = 4, typename T = uint64_t>
struct FilterBankDiscrete
{
    using P = Period<decim, T>;
    vector<complex<float>> state;
    vector<P> periods;
    float one_pole_a, one_pole_b;

    uint64_t frame;

    TimeSeriePhase phases_time_serie;

    FilterBankDiscrete(float period_start,float period_stop, int number, float alpha = 0.1) : frame(0)
    {
        assert(period_start < period_stop);
        const auto b = period_stop - period_start;
        for(int i = 0; i < number; i++){
            periods.push_back({period_start + i * b / static_cast<float>(number - 1)});
        }
        state.resize(number,{0,0});
        one_pole_b = std::powf(0.5,b * alpha);
        one_pole_a = 1.0 - one_pole_b;
    }

    FilterBankDiscrete() : frame(0) {}

    void process(const vector<float> &v)
    {
        for (size_t i = 0; i < v.size(); i++)
        {
            frame++;
            for (uint j = 0; j < periods.size(); j++)
            {
                float f = periods[j].getPhase(frame);
                auto c = GenerationFunctionSinLRU().get(f * 2 * M_PI);
                
                state[j] = state[j] * std::complex<float>(one_pole_b, 0) +
                    c * static_cast<std::complex<float>>(v[i] * one_pole_a);
            }
        }
        phases_time_serie.addSample(frame, state);
    }
    void addPeriod(const P &p)
    {
        periods.push_back(p);
    }

    void addperiod(std::vector<P> &p)
    {
        periods.insert(p.end(), p.begin(), p.end());
    }
};
