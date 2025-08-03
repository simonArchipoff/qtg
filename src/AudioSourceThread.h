#pragma once

#include <readerwriterqueue.h>
#include <rtaudio/RtAudio.h>
#include <atomic>
#include <vector>
#include <memory>
#include "SoundCardDrift.h"
#include "PeakDetector.h"
#include <readerwriterqueue.h>

#include "QuartzDSP.h"



class RtAudioCaptureThread : public PeakDetector {
public:
    const unsigned int sampleRate;
    unsigned int input_size;
    QuartzDSP_rt & dsp;
    std::vector<float> internal_buffer;
    RtAudioCaptureThread(QuartzDSP &dsp, int inputDeviceId = -1,
        unsigned int block_size = 64 * 1024, unsigned int number_channels = 2);

    ~RtAudioCaptureThread() {
        stop();
    }

    bool start();

    void stop();

    bool getDriftSoundcard(DriftResult &result);
    int getSampleRate()const {
        return sampleRate;
    }

    static std::vector<std::string> listInputDevices();

  public:
    DriftData soundcarddrift;
private:

    uint64_t currentFrame = 0;
    unsigned int channels;
    int inputDeviceId;

    std::atomic<bool> isRunning;
    std::unique_ptr<RtAudio> audio;


    moodycamel::ReaderWriterQueue<DriftResult> driftresultqueue;

    static int rtCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
        double /*streamTime*/, RtAudioStreamStatus status, void *userData);
};