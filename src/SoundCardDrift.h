#pragma once
#include <cstddef>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <readerwriterqueue.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "CircularBuffer.h"

#include "Constants.h"

struct TimeStamp
{
    uint64_t frame;
    timespec clock_tai, clock_realtime, clock_monotonic;
};

class TimeStampCSVWriter
{
  public:
    TimeStampCSVWriter(const std::string &filename) : ofs(filename)
    {
        if (!ofs)
            throw std::runtime_error("Unable to open CSV file: " + filename);
        write_header();
    }

    void write_all(const std::vector<TimeStamp> &data)
    {
        for (const auto &ts : data)
        {
            write_one(ts);
        }
    }

    void write_one(const TimeStamp &ts)
    {
        ofs << ts.frame << ',' << ts.clock_tai.tv_sec << ',' << ts.clock_tai.tv_nsec << ','
            << ts.clock_realtime.tv_sec << ',' << ts.clock_realtime.tv_nsec << ','
            << ts.clock_monotonic.tv_sec << ',' << ts.clock_monotonic.tv_nsec << '\n';
        ofs.flush();
    }

  private:
    std::ofstream ofs;

    void write_header()
    {
        ofs << "frame,"
            << "clock_tai_sec,clock_tai_nsec,"
            << "clock_realtime_sec,clock_realtime_nsec,"
            << "clock_monotonic_sec,clock_monotonic_nsec\n";
    }
};

struct DriftResult
{
    unsigned int sampleRate;           // Fréquence nominale (Hz)
    unsigned int numberPoints = 0;     // Nombre de points utilisés
    double dur_measures = 0.0; // Durée totale des mesures (en secondes)

    double drift_hz;   // Dérive absolue en Hz : fréquence réelle = sampleRate + delta
    double std_dev_hz; // Écart-type sur la dérive (Hz)
    double ci95_hz;    // Intervalle de confiance 95% (demi-largeur en Hz)

    double get_fps_estimated() const { return sampleRate + drift_hz; }

    double get_ppm() const { return 1e6 * drift_hz / sampleRate; }

    double get_spm() const { return drift_hz * Constants::SECONDS_PER_MONTH / sampleRate; }
    double get_spd() const {return drift_hz * Constants::SECONDS_PER_DAY / sampleRate; }

    double get_fps_ci95() const { return ci95_hz; }

    double get_spm_ci95() const { return ci95_hz * Constants::SECONDS_PER_MONTH / sampleRate; }
};

class DriftData
{
  public:
    DriftData(int n, int sampleRate) : circbuf(n), sampleRate(sampleRate), log("test.csv"), droped_value_init(3) {}
    void rt_insert_ts(uint64_t frame);

    bool execute()
    {
        TimeStamp t;
        bool n = false;
        while (ts_queue.try_dequeue(t))
        {
            if(droped_value_init == 0){ //cache warmup, I guess
                circbuf.push_back(t);
                log.write_one(t);
                n = true;
            } else {
                droped_value_init--;
            }
        }
        return n;
    }

    DriftResult getResult(size_t nb_points = 0, int clock =
#ifdef CLOCK_TAI
        CLOCK_TAI
#else
        CLOCK_REALTIME
#endif
    ) const;

    size_t getSize() const { return circbuf.size(); }

    CircularBuffer<TimeStamp> circbuf;
    moodycamel::ReaderWriterQueue<TimeStamp> ts_queue;
    int sampleRate;
    TimeStampCSVWriter log;
    int droped_value_init; // cold start problem
};

struct DriftEntry
{
    int64_t frame_init;
    timespec init_ts;
    int64_t frame_now;
    timespec now_ts;
    bool init_ = false;
    DriftEntry() : frame_init(0), frame_now(0), init_(false) {}

    void init(uint64_t frame);

    // Met à jour avec un nouveau frame et timestamp actuel
    void update(unsigned long current_frame);
    bool initialized() const;
    bool ready() const;
    // Calcule la divergence de fréquence entre init et now
    double fps_diff(uint64_t baseframerate) const;
};

std::ostream &operator<<(std::ostream &os, const DriftResult &result);