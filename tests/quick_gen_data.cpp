
#include <npy/tensor.h>
#include <npy/npy.h>
#include "FilterBank.h"
int f(){
    const int sr = 48000;
    const int nbfreq = 2;
    std::vector<size_t> shape({nbfreq,100*sr});
    npy::tensor<float> t(shape);

    vector<double> f;
    for(int i = 0; i < nbfreq; i++){
        f.push_back(20000.0+i/100.0);
    }
    SinGenBank foo(f,sr);
    for(uint i = 0; i < shape[1]; i++){
        for(uint j = 0; j < shape[0]; j++){
            t(j,i) = foo.get(j).real();
        }
        foo.step();
    }

    t.save("freqs.npy");
    return 0;
}

int g(){
    const int sr = 10;
    const int nbfreq2=7;
    const double frequency = 2;
    std::vector<size_t> shape({2*nbfreq2 + 1,10000*sr,2});
    npy::tensor<double> t(shape);
    SinGenBank gen({frequency + 0.005},sr);
    vector<double> f;

    for(int i = -nbfreq2 ; i <= nbfreq2 ; i++){
        f.push_back(frequency + i*0.05);
    }


    FilterBank foo(f,sr,100);
    for(uint i = 0; i < shape[1]; i++){
        vector<complex<double>> tmp = {gen.get(0)};
        foo.process(tmp);
        auto tmp2 = foo.getState();
        assert(tmp2.size() == shape[0]);
        for(uint j = 0; j < shape[0]; j++){
            t(j,i,0) = tmp2[j].real();
            t(j,i,1) = tmp2[j].imag();
        }
        gen.step(0);
    }
    t.save("state.npy");
    return 0;
}

int main(){
    return f();
}