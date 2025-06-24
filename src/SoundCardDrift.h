#pragma once
#include <ctime>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <vector>
#include <iostream>

struct DriftEntry {
    int64_t frame_init;
    timespec init_ts;
    int64_t frame_now;
    timespec now_ts;
    bool init_ = false;
    DriftEntry():frame_init(0),frame_now(0),init_(false){}

    void init(uint64_t frame);

    // Met à jour avec un nouveau frame et timestamp actuel
    void update(unsigned long current_frame) ;
    bool initialized()const ;
    bool ready() const ;
    // Calcule la divergence de fréquence entre init et now
    double fps_diff(uint64_t baseframerate) const ;
};


struct DriftResult {
    uint sampleRate;
    double average;
    double std_div;
    double ci95;
    bool ci95_set;
    double get_ppm(){
        return (1e6 * average) / sampleRate;
    }
    double get_spm(){
        return (average * 3600*24*(365/12.)) / sampleRate;
    }
    double get_spm_ci95(){
        return (ci95 * 3600*24*(365/12.)) / sampleRate;
    }
};
struct SoundCardDrift{
    std::vector<DriftEntry> diffentries;
    int nextEntry;

    SoundCardDrift(int size):diffentries(size),nextEntry(0){
    }

    void update(int frame);
    bool getResult(uint64_t sr,DriftResult&result);
};

std::ostream& operator<<(std::ostream& os, const DriftResult& result);