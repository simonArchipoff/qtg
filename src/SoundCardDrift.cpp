#include "SoundCardDrift.h"
#include <LinearRegression.h>
#include <iomanip>
#include <limits>
#include <cmath>
const static uint64_t Giga = 1'000'000'000ull;


struct DriftAnalysisResult
{
    double alpha;        // dérive relative (pente)
    double alpha_stderr;       // erreur standard sur la pente
    double fps_estimate; // fps réel estimé
    double fps_ci_lower; // intervalle de confiance bas
    double fps_ci_upper; // intervalle de confiance haut
    double r_squared;    // coefficient de détermination
};

std::ostream &operator<<(std::ostream &os, const DriftResult &result)
{
    os << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10)
       << "average: " << result.get_fps_estimated() << ", ic: " << result.get_fps_ci95();

    return os;
}
void DriftData::rt_insert_ts(uint64_t frame)
{
    TimeStamp t;
    t.frame = frame;
#ifdef CLOCK_TAI
    clock_gettime(CLOCK_TAI, &t.clock_tai);
#else
    std::memset(&t.clock_tai, 0, sizeof(t.clock_tai));
#endif
    clock_gettime(CLOCK_REALTIME, &t.clock_realtime);
    clock_gettime(CLOCK_MONOTONIC, &t.clock_monotonic);
    ts_queue.try_enqueue(t);
}

DriftResult DriftData::getResult(size_t nb_points, int clock) const
{
    if (nb_points == 0)
        nb_points = getSize();

    assert(nb_points <= getSize());

    std::vector<double> frames_sec, delta;
    frames_sec.reserve(nb_points - 1);
    delta.reserve(nb_points - 1);

    const TimeStamp &t0 = circbuf[-nb_points + 1];

    // Choix de l'horloge de référence pour t0
    timespec t0_system;
    switch (clock)
    {
#ifdef CLOCK_TAI
    case CLOCK_TAI:
        t0_system = t0.clock_tai;
        break;
#endif
    case CLOCK_REALTIME:
        t0_system = t0.clock_realtime;
        break;
    case CLOCK_MONOTONIC:
        t0_system = t0.clock_monotonic;
        break;
    default:
        abort();
    }

    for (int i = -nb_points + 2; i <= 0; ++i)
    {
        const TimeStamp &t = circbuf[i];
        double frame_ts = (t.frame - t0.frame) / static_cast<double>(sampleRate);

        timespec system;
        switch (clock)
        {
#ifdef CLOCK_TAI
        case CLOCK_TAI:
            system = t.clock_tai;
            break;
#endif
        case CLOCK_REALTIME:
            system = t.clock_realtime;
            break;
        case CLOCK_MONOTONIC:
            system = t.clock_monotonic;
            break;
        }

        double system_ts =
            (system.tv_sec - t0_system.tv_sec) + (system.tv_nsec - t0_system.tv_nsec) * 1e-9;

        double d = system_ts - frame_ts;

        frames_sec.push_back(frame_ts);
        delta.push_back(d);
    }

    auto fit = linear_regression(frames_sec, delta);

    DriftResult r;
    r.sampleRate = sampleRate;
    r.numberPoints = frames_sec.size();
    r.dur_measures = frames_sec.back() - frames_sec.front();
    r.drift_hz =  sampleRate / (1 + fit.alpha) - sampleRate;
    r.std_dev_hz = fit.alpha_stderr * sampleRate; 
    r.ci95_hz = fit.alpha_ci95 * sampleRate;

    return r;
}
