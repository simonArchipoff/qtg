#pragma once

#include <vector>
#include <complex>
#include <cmath>

using std::vector;
using std::complex;

class SinGenBank {
    vector<complex<double>> phase_shift_sample;
    vector<complex<double>> current_phase;
    vector<double> frequencies;
    double sampleRate = std::nan("undef");

public:
    SinGenBank()
    {
    }

    SinGenBank(const vector<double> & frequencies, double sampleRate)
    {    
        setSampleRate(sampleRate);
        for(auto i : frequencies){
            addFrequency(i);
        }
    }

    void setSampleRate(double fs) {
        sampleRate = fs;
        for (size_t i = 0; i < frequencies.size(); i++) {
            updateFrequencySamplerate(i);
        }
    }
    double getSampleRate() const{
        assert(!std::isnan(sampleRate));
        return sampleRate;
    }
    protected:
    void updateFrequencySamplerate(size_t idx) {
        assert(!std::isnan(sampleRate));
        assert(idx < frequencies.size());
        auto fs = sampleRate;
        double omega = 2.0 * M_PI * frequencies[idx] / fs;
        phase_shift_sample[idx] = std::exp(complex<float>(0.0, omega));
    }
    public:
    size_t addFrequency(double frequency){
        assert(check());
        auto tmp = frequencies.size();
        frequencies.push_back(frequency);
        current_phase.push_back({1, 0});
        phase_shift_sample.push_back({0, 0});
        updateFrequencySamplerate(tmp);
        return tmp;
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
    size_t getSizeBank() const{
        assert(check());
        return frequencies.size();
    }
    private:
    bool check() const{
        auto a = frequencies.size();
        auto b = current_phase.size();
        auto c = phase_shift_sample.size();
        return a == b && a == c;
    }
};

class FilterBank : private SinGenBank {
    vector<complex<float>> state;
    vector<float> tau;
    vector<float> one_pole_a, one_pole_b;
public:
    FilterBank(const vector<double> &freqs, double sampleRate, double tau = 20.0)
        : SinGenBank(){ 
            setSampleRate(sampleRate);
            for(auto f : freqs)
                addFrequency(f,tau);
          }
        
    template<typename T>
    const vector<complex<float>> & process(const vector<T> & input) {
        for (size_t i = 0; i < input.size(); i++) {
            auto &sin = get();
            auto sample = input[i];

            for (size_t j = 0; j < sin.size(); j++) {
                auto conv = conj(static_cast<complex<float>>(sin[j])) * static_cast<complex<float>>(sample);
                state[j] = state[j] * one_pole_b[j]
                           + conv * one_pole_a[j];
                SinGenBank::step(j);
            }
        }
        return state;
    }
    void setSampleRate(double sr){
        SinGenBank::setSampleRate(sr);
        for(size_t i = 0; i < getSizeBank(); i++){
            updateTau(i);
        }
    }

    int addFrequency(double freq, double tau){
        assert(check());
        auto f = SinGenBank::addFrequency(freq);
        this->tau.push_back(tau);
        this->one_pole_a.push_back(std::nan("unset pole_a"));
        this->one_pole_b.push_back(std::nan("unset pole_b"));
        this->state.push_back({0,0});
        assert(check());
        updateTau(f);
        return f;
    }

    vector<complex<float>> getState() const {
        return state;
    }

    protected:
    void updateTau(int i){
        auto fc = 1.0 / tau[i];
        one_pole_b[i] = std::exp(-2.0 * M_PI * fc / getSampleRate());
        one_pole_a[i] = 1.0 - one_pole_b[i];
    }

    private:
    bool check(){
        auto a = state.size();
        auto b = tau.size();
        auto c = one_pole_a.size();
        auto d = one_pole_b.size();

        return a == b
            && a == c
            && a == d
            && a == getSizeBank();
    }
};


