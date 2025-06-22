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
    DriftEntry():frame_init(-1),frame_now(-1){}

    void init(uint64_t frame);

    // Met à jour avec un nouveau frame et timestamp actuel
    void update(unsigned long current_frame) ;
    bool initialized()const ;
    bool ready() const ;
    // Calcule la divergence de fréquence entre init et now
    double fps_diff(uint64_t baseframerate) const ;
};


struct DriftResult {double average, std_div;};
struct SoundCardDrift{
    std::vector<DriftEntry> diffentries;
    int nextEntry;

    SoundCardDrift(int size):diffentries(size),nextEntry(0){
    }

    void update(int frame);
    bool getResult(uint64_t sr,DriftResult&result);
};

std::ostream& operator<<(std::ostream& os, const DriftResult& result);