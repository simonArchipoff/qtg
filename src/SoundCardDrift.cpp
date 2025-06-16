#include "SoundCardDrift.h"
#include <iomanip>
#include <limits>
const static uint64_t Giga = 1'000'000'000ull;

void DriftEntry::init(uint64_t frame)
{
    frame_init = frame;
    clock_gettime(CLOCK_REALTIME, &init_ts);
}

void DriftEntry::update(unsigned long current_frame)
{
    frame_now = current_frame;
    clock_gettime(CLOCK_REALTIME, &now_ts);
}
bool DriftEntry::initialized() const
{
    return frame_init > -1;
}
bool DriftEntry::ready() const
{
    return initialized() && frame_now > frame_init;
}

// Calcule la divergence de fréquence entre init et now
double DriftEntry::fps_diff(uint64_t baseframerate) const
{
    assert(ready());
    auto seconds = now_ts.tv_sec - init_ts.tv_sec;
    auto nanoseconds = now_ts.tv_nsec - init_ts.tv_nsec;
    uint64_t elapsed_nsec = seconds * Giga + nanoseconds;

    if (elapsed_nsec <= 0.0 || frame_now <= frame_init)
        return 0.0;
    uint64_t frames = frame_now - frame_init;
    __int128 numerator = static_cast<__int128>(frames) * Giga
                       - static_cast<__int128>(elapsed_nsec) * baseframerate;

    return static_cast<double>(numerator) / elapsed_nsec;


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


SoundCardDrift::DriftResult SoundCardDrift::getResult(uint64_t sr)
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
        return {0.0, 0.0};

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
    return {average, std_dev};
}



std::ostream& operator<<(std::ostream& os, const SoundCardDrift::DriftResult& result)
{
        os << std::fixed
       << std::setprecision(std::numeric_limits<double>::max_digits10)
       << "average: " << result.average << ", std_dev: " << result.std_div;

    return os;
}