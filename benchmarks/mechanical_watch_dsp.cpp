/**
 * watch_dsp — benchmark du pipeline DSP FrequencyCounter
 *
 * Affiche :
 *   - taux en échantillons/seconde
 *   - temps de traitement par buffer (moyenne, écart-type, min, max)
 *
 * Le signal d'entrée est un sinusoïde à 4 Hz (simulation d'une montre mécanique,
 * ~28800 alternances/heure). LO = 0. On mesure la vitesse du pipeline, pas la précision.
 *
 * Utilisation :
 *   ./benchmarks/watch_dsp --buffers 1000 --buffer-size 16 --sample-rate 48000
 *   ./benchmarks/watch_dsp --help
 */

#include <CLI/App.hpp>
#include <vector>
#include <cmath>
#include <iostream>

#include "FrequencyCounter.h"
#include "benchmark_timer.h"

/**
 * SignalSin — génère un sinusoïde pur à la fréquence donnée.
 * Pré-génère tous les échantillons pour que le benchmark ne mesure que le DSP.
 */
class SignalSin {
public:
    SignalSin(double sr, double freq)
        : sr_(sr)
        , freq_(freq)
        , phase_(0.0)
        , step_(2.0 * M_PI * freq / sr)
    {}

    std::vector<float> get(size_t n) {
        std::vector<float> out(n);
        for (size_t i = 0; i < n; i++) {
            out[i] = static_cast<float>(std::sin(phase_));
            phase_ += step_;
            if (phase_ > 2.0 * M_PI) phase_ -= 2.0 * M_PI;
        }
        return out;
    }

private:
    double sr_;
    double freq_;
    double phase_;
    double step_;
};

int main(int argc, char **argv) {
    CLI::App app{"Benchmark FrequencyCounter DSP pipeline"};

    double signal_duration = 10.0;
    size_t buffer_size   = 16;
    unsigned int sample_rate = 48000;
    int lo_freq          = 0;
    double drift_ppm     = 100;
    int harmonics        = 1;
    double duration_analysis = 1.0;
    double duration_between = 0.1;

    app.add_option("--signal-duration", signal_duration, "Durée du signal à traiter (s)")
       ->capture_default_str();
    app.add_option("-s,--buffer-size", buffer_size, "Taille du buffer (échantillons)")
       ->capture_default_str();
    app.add_option("-r,--sample-rate", sample_rate, "Taux d'échantillonnage (Hz)")
       ->capture_default_str();
    app.add_option("-l,--lo-freq", lo_freq, "Fréquence de l'oscillateur local (Hz)")
       ->capture_default_str();
    app.add_option("-d,--drift", drift_ppm, "Dérive du quartz simulé (ppm)")
       ->capture_default_str();
    app.add_option("-H,--harmonics", harmonics, "Harmoniques à analyser")
       ->capture_default_str();
    app.add_option("--duration", duration_analysis, "Durée d'analyse (s)")
       ->capture_default_str();
    app.add_option("--duration-between", duration_between, "Intervalle entre analyses (s)")
       ->capture_default_str();

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // Configuration
    FrequencyCounterConfig config;
    config.sample_rate_nominal = sample_rate;
    config.lo_freq             = lo_freq;
    config.analytic_signal     = true;
    config.duration_analysis_s = duration_analysis;
    config.harmonics           = harmonics;
    config.duration_between_analysis = duration_between;

    // Signal sinusoïde à 4 Hz (simulation montre mécanique, ~28800 alternances/heure)
    const double WATCH_FREQ = 4.0;
    SignalSin sig(sample_rate, WATCH_FREQ);

    // Pré-générer le signal d'entrée
    size_t total_samples = static_cast<size_t>(signal_duration * sample_rate);
    std::vector<float> input;
    input.reserve(total_samples);

    auto pre_start = std::chrono::steady_clock::now();

    size_t remaining = total_samples;
    while (remaining > 0) {
        size_t chunk = std::min(remaining, static_cast<size_t>(4096));
        auto block = sig.get(chunk);
        input.insert(input.end(), block.begin(), block.end());
        remaining -= chunk;
    }

    auto pre_end = std::chrono::steady_clock::now();
    double pre_time_ms = std::chrono::duration<double, std::milli>(pre_end - pre_start).count();
    std::cout << "[bench]  pre-generated " << input.size() << " samples in " << pre_time_ms << " ms\n";

    // Initialise le DSP
    FrequencyCounterDSP dsp(config);
    dsp.rt.init(buffer_size);

    // Benchmark
    BenchTimer timer("FrequencyCounterDSP rt_process + runAsync");

    size_t idx = 0;
    size_t total_buffers = total_samples / buffer_size;
    for (size_t i = 0; i < total_buffers; i++) {
        std::vector<float> block(input.begin() + idx, input.begin() + idx + buffer_size);
        idx += buffer_size;

        auto t0 = std::chrono::steady_clock::now();
        dsp.rt.rt_process(block);
        dsp.runAsync();
        auto t1 = std::chrono::steady_clock::now();

        double elapsed_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        timer.tick(buffer_size);

        dsp.setRealSR(static_cast<double>(sample_rate));
    }

    timer.report();

    // Résultat final
    Result result;
    if (dsp.getResult(result)) {
        auto freq = result.strongest_frequency();
        std::cout << "[result]  frequency: " << freq << " Hz" << std::endl;
    }

    return 0;
}
