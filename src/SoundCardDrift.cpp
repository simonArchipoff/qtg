#include "SoundCardDrift.h"
#include <iomanip>
#include <limits>
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
    double elapsed_sec =  seconds + nanoseconds / 1e9;
    int64_t frames = frame_now - frame_init;
    double numerator = frames - elapsed_sec * baseframerate;
    assert(frames > 0);
    assert(elapsed_sec > 0);

    return numerator / elapsed_sec;


    

    //return static_cast<double>(frames * Giga  - elapsed_nsec * baseframerate ) / elapsed_nsec; // === (frame / elapsed_sec) - baseframerate
}

void SoundCardDrift::update(int frame)
{
    auto &entry = diffentries[nextEntry];
    if (entry.initialized())
    {
        entry.update(frame);
    }
    else
    {
        entry.init(frame);
    }
    nextEntry += 1;
    nextEntry %= diffentries.size();
}
static double t_score_95(size_t df) {
    // Valeurs approximatives pour quelques degrés de liberté
    if (df >= 100) return 1.984;  // approx pour df >= 100
    if (df >= 30) return 2.042;
    if (df >= 20) return 2.086;
    if (df >= 10) return 2.228;
    if (df >= 5)  return 2.571;
    return 12.71; // df = 1
}

bool SoundCardDrift::getResult(uint64_t sr, DriftResult & result)
{
    std::vector<double> diffs;
    diffs.reserve(diffentries.size());

    // Première passe : collecter les fps_diff valides
    for (const auto &entry : diffentries)
    {
        if (entry.ready())
        {
            diffs.push_back(entry.fps_diff(sr));
        }
    }

    size_t count = diffs.size();
    if (count == 0)
        return false;

    // Moyenne
    double sum = 0.0;
    for (double d : diffs)
        sum += d;

    double average = sum / count;

    double variance = 0.0;
    if (count >= 2)
    {
        for (double d : diffs)
            variance += (d - average) * (d - average);
        variance /= (count - 1);
    }
    double std_dev = std::sqrt(variance);

    // i am not good a this, I blame chatgpt if it's wrongm
    double ci95 = 0.0;
    if (count >= 2)
    {
        double t = t_score_95(count - 1);
        ci95 = t * (std_dev / std::sqrt(static_cast<double>(count)));
        result.ci95_set = true;
        result.ci95 = ci95;
    }


    result.sampleRate = sr;
    result.average = average;
    result.std_div = std_dev;
    return true;
}



std::ostream& operator<<(std::ostream& os, const DriftResult& result)
{
        os << std::fixed
       << std::setprecision(std::numeric_limits<double>::max_digits10)
       << "average: " << result.average << ", std_dev: " << result.std_div;

    return os;
}