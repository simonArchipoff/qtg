#pragma once
#include <complex>
#include <vector>
#include <cassert>
using std::vector;
using std::complex;
using std::numeric_limits;
template<int decim>
struct Period{
    uint period;
    template<typename T>
    Period(T f):period(f * (1 << decim)){
        assert( f * (1<<decim) <= numeric_limits<decltype(period)>::max());
    }
    float getPeriod() const{
        return static_cast<float>(period) / (1<<decim);
    }
    int getPhase(u_int64_t i, int normFactor){
        assert(i*(1<<decim) <= numeric_limits<decltype(i)>::max());
        return ((i * (1 << decim)) % period) * normFactor
                / period;
    }
    float getPhase(u_int64_t i){
        assert(i*(1<<decim) <= numeric_limits<decltype(i)>::max());

        return (i * (1 << decim)) % period
                / static_cast<float>(period);
    }
};

template<int decim>
u_int64_t lcmv(vector<Period<decim>> & v){
    if (v.empty())
        return 0;
    u_int64_t result = v[0].period;
    for (size_t i = 1; i < v.size(); ++i) {
        result = std::lcm(result, static_cast<u_int64_t>(v[i].period));
    }
    return result;
};


struct GeneratorFunction{
    std::complex<float> get(float phase);
};

struct GenerationFunctionSin : GeneratorFunction{
    std::complex<float> get(float phase){
        float s,c;
        sincosf(phase,&s,&c);
        return std::complex<float>(c,s);
    }
};

template<int decim=4>
struct FilterBankDiscrete{
    uint sampleRate;
    vector<complex<float>> state;
    vector<Period<decim>> period;
    vector<float> tau;


    u_int64_t frame;
    u_int64_t modulo;

    FilterBankDiscrete(uint sampleRate):sampleRate(sampleRate),frame(0),modulo(0){

    }

    void process(vector<float> * v)
    {
        for(int i = 0; i < v.size(); i++)
        {
            frame++;
            frame %= modulo;
            for(uint j = 0; j < period.size(); j++)
            {
                float f = period[j].getPhase(frame);

            }
        }
    
    }
    void addPeriod(const Period<4>& p) {
        period.push_back(p);
        modulo = lcmv<decim>(period);
    }

    void addperiod(std::vector<Period<4>>& p) {
        period.insert(p.end(), p.begin(), p.end());
        modulo = lcmv<decim>(period);
    }
};