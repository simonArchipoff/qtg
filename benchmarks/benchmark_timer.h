#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <algorithm>

/**
 * BenchTimer — mesure temps de traitement, ecart-type, taux en échantillons/seconde.
 *
 * Usage :
 *   BenchTimer timer("ma_fonction");
 *   for (...) {
 *       timer.tick(buffer_size);
 *       ma_fonction(buffer);
 *   }
 *   timer.report();
 */
class BenchTimer {
public:
    explicit BenchTimer(const std::string &name = "")
        : name_(name)
        , start_(std::chrono::steady_clock::now())
        , count_(0)
        , total_samples_(0)
        , total_time_us_(0)
    {}

    /**
     * Appelée APRÈS le traitement d'un buffer.
     * @param samples nombre d'échantillons traités dans ce buffer.
     */
    void tick(size_t samples) {
        auto now = std::chrono::steady_clock::now();
        double elapsed_us = std::chrono::duration<double, std::micro>(now - start_).count();

        times_us_.push_back(elapsed_us);
        total_samples_ += samples;
        total_time_us_ += elapsed_us;
        count_++;

        start_ = now;
    }

    /**
     * Affiche un résumé formaté sur stdout.
     */
    void report() const {
        if (count_ == 0) {
            std::cerr << "[bench] " << name_ << ": no data\n";
            return;
        }

        double total_s   = total_time_us_ / 1'000'000.0;
        double mean_us   = total_time_us_ / count_;
        double rate = static_cast<double>(total_samples_) / total_s; // samples/s

        // écart-type (pop)
        double sq_sum = 0.0;
        for (double t : times_us_) {
            double d = t - mean_us;
            sq_sum += d * d;
        }
        double stddev_us = std::sqrt(sq_sum / count_);

        // min / max
        auto [tmin, tmax] = std::minmax_element(times_us_.begin(), times_us_.end());

        // Format rate
        double rate_val;
        const char *rate_unit;
        if (rate >= 1e6) {
            rate_val = rate / 1e6;
            rate_unit = "M samples/s";
        } else if (rate >= 1e3) {
            rate_val = rate / 1e3;
            rate_unit = "k samples/s";
        } else {
            rate_val = rate;
            rate_unit = "samples/s";
        }

        // Format time units (ms, μs, or ns) based on magnitude
        auto [mean_val, time_unit] = format_duration(mean_us);
        double stddev_val = format_duration(stddev_us).first;
        double min_val    = format_duration(*tmin).first;
        double max_val    = format_duration(*tmax).first;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3);
        ss << "[bench] " << name_ << "\n"
           << "  total time:  " << std::setprecision(2) << total_s << " s\n"
           << "  rate:        " << std::setprecision(3) << rate_val << " " << rate_unit << "\n"
           << "  time/buffer: " << std::setprecision(3) << mean_val << " " << time_unit
           << " (stddev " << std::setprecision(3) << stddev_val << " " << time_unit << ")\n"
           << "  min:         " << std::setprecision(3) << min_val << " " << time_unit << "\n"
           << "  max:         " << std::setprecision(3) << max_val << " " << time_unit;

        std::cout << ss.str() << std::endl;
    }

    /** Retourne le vecteur brut des temps (microsecondes) — pour usage externe. */
    const std::vector<double> &times() const { return times_us_; }

    size_t count() const { return count_; }

private:
    static std::pair<double, const char *> format_duration(double us) {
        if (us >= 1000.0) return {us / 1000.0, "ms"};
        if (us >= 1.0)    return {us, "μs"};
        return {us * 1000.0, "ns"};
    }

    std::string name_;
    std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
    size_t count_ = 0;
    size_t total_samples_ = 0;
    double total_time_us_ = 0;
    std::vector<double> times_us_;
};
