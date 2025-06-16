#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <iostream>
class GoertzelBank {
public:
    GoertzelBank(double fs, const std::vector<double>& freqs, size_t blockSize)
        : fs_(fs), N_(blockSize), numFreqs_(freqs.size()),
          coeffs_(numFreqs_), q1_(numFreqs_, 0.0), q2_(numFreqs_, 0.0), power_(numFreqs_, 0.0) {

        for (size_t i = 0; i < numFreqs_; ++i) {
            double k = 0.5 + (N_ * freqs[i]) / fs_;
        
            std::cout << std::setprecision(50) << freqs[i] << " " << k << std::endl;
            double omega = 2.0 * M_PI * k / N_;
            coeffs_[i] = 2.0 * std::cos(omega);
        }
        reset();
    }

    void processBlock(const std::vector<double> input) {
        #pragma omp parallel for
        for (size_t i = 0; i < numFreqs_; ++i) {
            double q1 = q1_[i], q2 = q2_[i];
            double coeff = coeffs_[i];

            for (size_t n = 0; n < input.size(); ++n) {
                double x = input[n];
                double q0 = coeff * q1 - q2 + x;
                q2 = q1;
                q1 = q0;
            }

            q1_[i] = q1;
            q2_[i] = q2;

            double pwr = q1 * q1 + q2 * q2 - q1 * q2 * coeff;
            power_[i] = pwr;
        }
    }

    // Accès aux puissances courantes (valeurs mises à jour à chaque bloc)
    const std::vector<double>& getMagnitudes() const {
        return power_;
    }

    // Réinitialise l'état interne (utile en début de traitement ou reset manuel)
    void reset() {
        std::fill(q1_.begin(), q1_.end(), 0.0);
        std::fill(q2_.begin(), q2_.end(), 0.0);
        std::fill(power_.begin(), power_.end(), 0.0);
    }

private:
    double fs_;
    size_t N_;
    size_t numFreqs_;

    std::vector<double> coeffs_;
    std::vector<double> q1_, q2_;
    std::vector<double> power_;
};
