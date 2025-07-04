#pragma once
#include <vector>
#include "Constants.h"
#include <cassert>
struct Result{
    std::vector<double> magnitudes;
    std::vector<double> phases;
    std::vector<double> frequencies;
    double time;
    double progress= 0.0;
    double nominal_frequency;
    double getProgressPercent(){
        return progress * 100;
    }
    double  correction_spm = 0.0;

    std::vector<double> sec_per_month_corrected()const {
        auto f = sec_per_month();
        for(auto & i : f){
            i += correction_spm;
        }
        return f;
    }

    std::vector<double> sec_per_month() const {
        std::vector<double> res(frequencies.size());
        for(unsigned long i = 0 ; i < res.size(); i++){
            res[i] = ((frequencies[i] - nominal_frequency) * Constants::SECONDS_PER_MONTH) / nominal_frequency; 
        }
        return res;
    }
};
