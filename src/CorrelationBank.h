#pragma once
#include <vector>
#include <cmath>
#include <cassert>
#include <omp.h>

#include <ResultSignal.h>

class CorrelationBank {
public:
    void init(double fs, const std::vector<double>& freqs)
    {
        fs_ = fs;
        numFreqs_ = freqs.size();
        freqs_ = freqs;
        realSums.assign(numFreqs_, 0.0);
        imagSums.assign(numFreqs_, 0.0);
        two_pi_f_over_fs.resize(numFreqs_);
        currentFrame = 0;
        for (size_t i = 0; i < numFreqs_; ++i) {
            two_pi_f_over_fs[i] = 2.0 * M_PI * freqs[i] / fs_;
        }
    }
    CorrelationBank(){}

    void reset(){
        currentFrame = 0;
        std::fill(realSums.begin(),realSums.end(),0.0);
        std::fill(imagSums.begin(), imagSums.end(), 0.0);
    }

    std::vector<double> getFrequencies() const{
        return std::vector<double>(freqs_.begin(), freqs_.end());
    }

    void processBlock(const std::vector<float>& input) {
        omp_set_num_threads(8);
        #pragma omp parallel for
        for (size_t i = 0; i < numFreqs_; i++) {

            double freq = freqs_[i];

            for (size_t n = 0; n < input.size(); n++) {
                double angle = two_pi_f_over_fs[i] * (currentFrame + n);
                double sample = input[n];
                
                double s,c;

                sincos(angle,&s,&c);
                realSums[i] += sample * c;
                imagSums[i] += -sample * s; // conjugué
            }
        }

        currentFrame += input.size();
    }
    template<int N>
    void processBlock(const std::vector<float>& input) {
        omp_set_num_threads(8);
        assert(input.size() == N);
        #pragma omp parallel for
        for (size_t i = 0; i < numFreqs_; i++) {

            double freq = freqs_[i];

            for (size_t n = 0; n < N; n++) {
                double angle = two_pi_f_over_fs[i] * (currentFrame + n);
                double sample = input[n];
                
                double s,c;
                sincos(angle,&s,&c);

                realSums[i] += sample * c;
                imagSums[i] += -sample * s; // conjugué
            }
        }

        currentFrame += N;
    }

   
    std::vector<double> getMagnitudes() const {
        std::vector<double> magnitudes_(numFreqs_,0.0);
        for (size_t i = 0; i < numFreqs_; ++i) {
            double r = realSums[i];
            double im = imagSums[i];
            magnitudes_[i] = std::sqrt(r * r + im * im) / currentFrame;
        }
        return magnitudes_;
    }

    std::vector<double> getPhases() const {
        std::vector<double> phases_(numFreqs_,0.0);
        for (size_t i = 0; i < numFreqs_; ++i) {
            double r = realSums[i];
            double im = imagSums[i];
            phases_[i] = std::atan2(im, r);
        }
        return phases_;
    }

    double getTime() const{
        return 1.0 * currentFrame / fs_;
    }

    Result getResult(){
        return Result {getMagnitudes(), getPhases(), getFrequencies(), getTime()};
    }

private:
    double fs_;
    int currentFrame;
    size_t numFreqs_;
    std::vector<double> freqs_;
    std::vector<double> two_pi_f_over_fs;
    std::vector<double> realSums;
    std::vector<double> imagSums;
};