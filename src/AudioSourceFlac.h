
#include "AudioSourceThread.h"
#include <sndfile.h>

/* 
class AudioSourceFlac : public AudioSourceThread {
public:
    AudioSourceFlac(const std::string& path):AudioSourceThread(){
        SF_INFO sfinfo = {};
        sndfile = sf_open(path.c_str(), SFM_READ, &sfinfo);
        if (!sndfile) throw std::runtime_error("Failed to open FLAC file");

        channels = sfinfo.channels;
        sampleRate = sfinfo.samplerate;
    }
    void init(const std::vector<double>& freqs){
        this->correlationBank.init(sampleRate,freqs);
    }
    ~AudioSourceFlac() {
        if (sndfile) sf_close(sndfile);
    }

    bool next_buffer(std::vector<float>& buffer) override {
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::vector<float> tmp(buffer.size() * channels);
        sf_count_t read = sf_readf_float(sndfile, tmp.data(), buffer.size());
        if (read != (buffer.size() ))
            return false;

        // mix to mono
        for (size_t i = 0; i < buffer.size(); ++i) {
            float sum = 0;
            for (int c = 0; c < channels; ++c) {
                sum += tmp[i * channels + c];
            }
            buffer[i] = sum / channels;
        }

        return true;
    }

private:
    SNDFILE* sndfile = nullptr;
    int channels = 0;
    int sampleRate = 0;
};*/