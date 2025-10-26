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

public:
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

    inline const vector<complex<double>> & get() const {
        return current_phase;
    }
    std::complex<double> get(int i) const {
        return current_phase[i];
    }

    std::complex<double> step(int i) {
        return current_phase[i] *= phase_shift_sample[i];
    }
    void step(){
        for(uint i = 0; i < current_phase.size(); i++){
            step(i);
        }
    }
};

class FilterBank : private SinGenBank {
    vector<complex<double>> state;
    double tau;
    double one_pole_a, one_pole_b;
    double sr;
public:
    FilterBank(const vector<double> &freqs, double sampleRate, double tau = 20.0)
        : SinGenBank(freqs, sampleRate),
          state(freqs.size(), complex<double>(0.0, 0.0)),
          sr(sampleRate) {
            this->tau = tau;
            setTau(tau);
          }

    template<typename T>
    const vector<complex<double>> & process(const vector<T> & input) {
        for (size_t i = 0; i < input.size(); i++) {
            auto &sin = get();
            auto sample = input[i];

            for (size_t j = 0; j < sin.size(); j++) {
                auto conv = conj(sin[j]) * static_cast<complex<double>>(sample);
                state[j] = state[j] * one_pole_b
                           + conv * one_pole_a;
                step(j);
            }
        }
        return state;
    }
    void setSampleRate(double sr){
        SinGenBank::setSampleRate(sr);
        this->sr = sr;
        setTau(this->tau);
    }

    void setTau(double tau) {
        this->tau = tau;
        auto fc = 1.0 / tau;
        one_pole_b = std::exp(-2.0 * M_PI * fc / sr);
        one_pole_a = 1.0 - one_pole_b;
    }

    vector<complex<double>> getState(){
        return state;
    }

};


