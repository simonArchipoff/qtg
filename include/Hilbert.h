#pragma once
#include <Butterworth.h>
#include <cstddef>
#include <cassert>

struct Hilbert {
    unsigned int sr;
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<12>, 2> lowpass;

    Hilbert() : sr(0) {}

    Hilbert(unsigned int sampleRate) : sr(sampleRate) {
        init(sampleRate);
    }

    void init(unsigned int sampleRate) {
        sr = sampleRate;
        lowpass.setup(12 /* order */, sr, sr / 4.0);
    }

    void process(std::size_t size, float* input, float* output_r, float* output_i) {
        assert(size % 4 == 0);
        assert(sr != 0);

        for (std::size_t i = 0; i < size; i += 4) {
            // Modulation par exp(-j·π/2·n) : séquence 1, -j, -1, j
            output_r[i + 0] =  input[i + 0];   // *1
            output_i[i + 0] =  0.0f;

            output_r[i + 1] =  0.0f;           // *(-j)
            output_i[i + 1] = -input[i + 1];

            output_r[i + 2] = -input[i + 2];   // *(-1)
            output_i[i + 2] =  0.0f;

            output_r[i + 3] =  0.0f;           // *j
            output_i[i + 3] =  input[i + 3];

            // Filtrage passe-bas à sr/4 (supprime les hautes fréquences)
            float* c[] = {output_r + i, output_i + i};
            lowpass.process(4, c);

            // Démodulation par exp(j·π/2·n) : séquence 1, j, -1, -j
            // i+1 : multiplication par j → (-b + j a)
            float tmp_r1 = output_r[i + 1];
            float tmp_i1 = output_i[i + 1];
            output_r[i + 1] = -tmp_i1;
            output_i[i + 1] =  tmp_r1;

            // i+2 : multiplication par -1 → (-a - j b)
            output_r[i + 2] = -output_r[i + 2];
            output_i[i + 2] = -output_i[i + 2];

            // i+3 : multiplication par -j → (b - j a)
            float tmp_r3 = output_r[i + 3];
            float tmp_i3 = output_i[i + 3];
            output_r[i + 3] =  tmp_i3;
            output_i[i + 3] = -tmp_r3;
        }
    }
};
