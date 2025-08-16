#pragma once

#include <vector>
#include <complex>
#include <cmath>

using std::vector;
using std::complex;

class SinGenBank {
    vector<complex<double>> phase_shift_sample;
    vector<complex<double>> current_phase;
    const vector<double> frequencies;

protected:
    SinGenBank(const vector<double> & frequencies, double sampleRate)
        : phase_shift_sample(frequencies.size()),
          current_phase(frequencies.size(), complex<double>(1.0, 0.0)),
          frequencies(frequencies) 
    {
        setSampleRate(sampleRate);
    }

    void setSampleRate(double fs) {
        for (size_t i = 0; i < frequencies.size(); i++) {
            double omega = 2.0 * M_PI * frequencies[i] / fs;
            phase_shift_sample[i] = std::exp(complex<double>(0.0, omega));
        }
    }

    inline const vector<complex<double>> & getSin() const {
        return current_phase;
    }

    void step() {
        for (size_t i = 0; i < phase_shift_sample.size(); i++) {
            current_phase[i] *= phase_shift_sample[i];
        }
    }
};

class FilterBank : private SinGenBank {
    vector<complex<double>> state;
    double one_pole_a, one_pole_b;
    double sr;
public:
    FilterBank(const vector<double> &freqs, double sampleRate, double tau = 30.0)
        : SinGenBank(freqs, sampleRate),
          state(freqs.size(), complex<double>(0.0, 0.0)),
          sr(sampleRate) {
            setTau(tau);
          }

    template<typename T>
    const vector<complex<double>> & process(const vector<T> & input) {
        for (size_t i = 0; i < input.size(); i++) {
            auto &sin = getSin();
            auto sample = input[i];
            for (size_t j = 0; j < sin.size(); j++) {
                state[j] = state[j] * one_pole_b
                           + sin[j] * static_cast<complex<double>>(sample) * one_pole_a;
            }
            step();
        }
        return state;
    }
    void setSampleRate(double sr){
        SinGenBank::setSampleRate(sr);
        this->sr = sr;
    }

    void setTau(double tau) {
        auto fc = 1.0 / tau;
        one_pole_b = std::exp(-2.0 * M_PI * fc / sr);
        one_pole_a = 1.0 - one_pole_b;
    }  
};

class PhaseDrift{
    vector<double> last_phase;
    double last_time;
};