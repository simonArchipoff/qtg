#include "SoundCardDrift.h"
#include <iomanip>
#include <limits>
#include <cmath>
const static uint64_t Giga = 1'000'000'000ull;

void DriftEntry::init(uint64_t frame)
{
    this->init_ = true;
    frame_init = frame;
    clock_gettime(CLOCK_TAI, &init_ts);
}

void DriftEntry::update(unsigned long current_frame)
{
    frame_now = current_frame;
    clock_gettime(CLOCK_TAI, &now_ts);
}
bool DriftEntry::initialized() const
{
    return init_;
}
bool DriftEntry::ready() const
{
    return initialized() && frame_now > frame_init;
}

// Calcule la divergence de fréquence entre init et now
double DriftEntry::fps_diff(uint64_t baseframerate) const
{
    assert(ready());
    int64_t seconds = now_ts.tv_sec - init_ts.tv_sec;
    int64_t nanoseconds = now_ts.tv_nsec - init_ts.tv_nsec;
    double elapsed_sec = seconds + nanoseconds / 1e9;
    int64_t frames = frame_now - frame_init;
    double numerator = frames - elapsed_sec * baseframerate;
    assert(frames > 0);
    assert(elapsed_sec > 0);

    return numerator / elapsed_sec;

    // return static_cast<double>(frames * Giga  - elapsed_nsec * baseframerate )
    // / elapsed_nsec; // === (frame / elapsed_sec) - baseframerate
}

/*
  code chatgpt pour la reg. lin.

*/
// Approximation de la valeur t* pour un IC à 95% selon N
double t_critical_95(size_t df)
{
    // Pour N > 30, t ≈ 1.96
    if (df >= 100)
        return 1.984;
    if (df >= 50)
        return 2.009;
    if (df >= 30)
        return 2.042;
    if (df >= 20)
        return 2.086;
    if (df >= 10)
        return 2.228;
    if (df >= 5)
        return 2.571;
    return 12.706; // df = 1
}

struct LinearFitWithCI
{
    double alpha;
    double beta;
    double r_squared;
    double alpha_stderr;
    double alpha_ci;
};

std::ostream &operator<<(std::ostream &os, const LinearFitWithCI &fit)
{
    os << std::fixed << std::setprecision(10); // Affichage avec 10 décimales
    os << "Pente (alpha)       : " << fit.alpha << "\n"
       << "Décalage (beta)     : " << fit.beta << "\n"
       << "Erreur std (alpha)  : " << fit.alpha_stderr << "\n"
       << "IC 95% (alpha)      : " << fit.alpha_ci << "\n"
       << "Coefficient R²      : " << fit.r_squared;
    return os;
}

struct DriftAnalysisResult
{
    double alpha;        // dérive relative (pente)
    double stderr;       // erreur standard sur la pente
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

struct LinearFitResult
{
    double alpha;        // pente
    double beta;         // intercept
    double r_squared;    // coefficient de détermination
    double alpha_stderr; // erreur standard sur alpha
    double alpha_ci95;
};
double get_t_critical_95(int dof)
{
    assert(dof > 0);
    if (dof >= 30) return 2.0;  // approximation grossière pour dof > 30
    
    // Sinon, on peut stocker quelques valeurs critiques en dur, par exemple:
    // dof : valeur t pour 95% bilateral
    static const double t_table[] = {
        12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
        2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
        2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042
    };
    int index = dof - 1;
    if (index >= 30) return 2.0;
    if (index < 0) index = 0;
    return t_table[index];
}
LinearFitResult linear_regression(const std::vector<double> &x, const std::vector<double> &y)
{
    assert(x.size() == y.size());
    size_t N = x.size();
    if (N < 2)
        throw std::invalid_argument("Au moins 2 points nécessaires");

    double sum_x = 0, sum_y = 0;
    double sum_xx = 0, sum_xy = 0, sum_yy = 0;

    for (size_t i = 0; i < N; ++i)
    {
        sum_x += x[i];
        sum_y += y[i];
        sum_xx += x[i] * x[i];
        sum_xy += x[i] * y[i];
        sum_yy += y[i] * y[i];
    }

    double mean_x = sum_x / N;
    double mean_y = sum_y / N;

    double Sxx = sum_xx - N * mean_x * mean_x;
    double Sxy = sum_xy - N * mean_x * mean_y;
    double Syy = sum_yy - N * mean_y * mean_y;

    double alpha = Sxy / Sxx;
    double beta = mean_y - alpha * mean_x;

    // Calcul des résidus
    double ss_res = 0;
    for (size_t i = 0; i < N; ++i)
    {
        double y_pred = alpha * x[i] + beta;
        double diff = y[i] - y_pred;
        ss_res += diff * diff;
    }

    // Variance résiduelle
    double variance = ss_res / (N - 2);

    // Erreur standard sur la pente alpha
    double alpha_stderr = std::sqrt(variance / Sxx);

    // Coefficient de détermination R²
    double r_squared = 1 - (ss_res / Syy);
    double t_crit = get_t_critical_95(static_cast<int>(N - 2));
    double alpha_ci95 = t_crit * alpha_stderr;
    return {alpha, beta, r_squared, alpha_stderr, alpha_ci95};
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
    case CLOCK_TAI:
        t0_system = t0.clock_tai;
        break;
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
        case CLOCK_TAI:
            system = t.clock_tai;
            break;
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
