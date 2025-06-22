#pragma once
#include <vector>

struct Result{
    std::vector<double> magnitudes;
    std::vector<double> phases;
    std::vector<double> frequencies;
    double time;

    std::vector<double> sec_per_month() const{
        std::vector<double> res(frequencies.size());
        for(int i = 0 ; i < res.size(); i++){
            res[i] = ((frequencies[i] - 32768) * 3600*24*30.5)/32768; 
        }
        return res;
    }
};
