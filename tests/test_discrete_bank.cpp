#include <FilterBankDiscrete.h>


int main(){

    FilterBankDiscrete<16> filter(1,1.1,128);
    vector<float> foo(1,10);
    filter.process({10});
    for(size_t i = 0; i < 1024; i++){
        filter.process({0});
        auto r = filter.state;
    }

    return 0;

}