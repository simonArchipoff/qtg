#pragma once

#include <rtaudio/RtAudio.h>
#include <atomic>
#include <vector>
#include <memory>
#include "SoundCardDrift.h"

#include <iostream>

#include "QuartzDSP.h"




class RtAudioCaptureThread {
public:
    const uint sampleRate;
    uint input_size;
    QuartzDSP_rt & dsp;
    std::vector<float> internal_buffer;     
    RtAudioCaptureThread(QuartzDSP & dsp,
                        int inputDeviceId = -1,
                        uint block_size=8 * 1024,
                        uint number_channels=2
                        )
         :sampleRate(dsp.config.sample_rate),
         input_size(block_size),
         dsp(dsp.rt),
         soundcarddrift(1024*1024,sampleRate),
         channels(number_channels),
         inputDeviceId(inputDeviceId),
         isRunning(false),
         audio(nullptr)
    {
    }

    ~RtAudioCaptureThread() {
        stop();
    }

    bool start() {
    if (isRunning.load()) return false;

        audio = std::make_unique<RtAudio>();

        if (audio->getDeviceCount() < 1) {
            std::cerr << "No audio devices found!\n";
            return false;
        }

        RtAudio::StreamParameters iParams;

        iParams.deviceId = (inputDeviceId >= 0) ? inputDeviceId : audio->getDefaultInputDevice();
        iParams.nChannels = channels;
        iParams.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME ;

        audio->openStream(nullptr, &iParams, RTAUDIO_FLOAT32,
                        sampleRate, &this->input_size, &RtAudioCaptureThread::rtCallback, this, &options);
        internal_buffer.resize(input_size);
        dsp.init(input_size);
        isRunning.store(true);
        audio->startStream();

        return true;
    }

    void stop() {
        if (!isRunning.load()) return;

        isRunning.store(false);
        if (audio && audio->isStreamOpen()) {
            audio->stopStream();
            audio->closeStream();
        }
        audio.reset();
    }


    bool getDriftSoundcard(DriftResult & result){
        bool good = false;
        DriftResult r;
        while(driftresultqueue.try_dequeue(r)){
            result = r;
            good = true;
        }
        return good;
    }
    int getSampleRate()const {
        return sampleRate;
    }


    static std::vector<std::string> listInputDevices() {
        std::vector<std::string> deviceList;
        RtAudio audio;
        auto devices = audio.getDeviceIds();

        for (auto & i : devices) {
            RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
            if (info.inputChannels > 0) {
                std::ostringstream oss;
                oss << "[" << i << "] " << info.name
                    << " (" << info.inputChannels << " in / "
                    << info.outputChannels << " out)";
                deviceList.push_back(oss.str());
            }
        }
        
        return deviceList;
    }

public:
    DriftData soundcarddrift;
private:

    uint64_t currentFrame = 0;
    unsigned int channels;
    int inputDeviceId;

    std::atomic<bool> isRunning;
    std::unique_ptr<RtAudio> audio;


    moodycamel::ConcurrentQueue<DriftResult> driftresultqueue;


    static int rtCallback(void* outputBuffer, void* inputBuffer,
                          unsigned int nFrames, double /*streamTime*/,
                          RtAudioStreamStatus status, void* userData)
    {
        (void) outputBuffer;
        auto* self = static_cast<RtAudioCaptureThread*>(userData);
        
        if (status){
            std::cerr << "Stream underflow/overflow detected." << std::endl;
            exit(1);
        }
        if (!inputBuffer) return 0;
        
        self->currentFrame += nFrames;

        self->soundcarddrift.rt_insert_ts(self->currentFrame);

        const float* in = static_cast<const float*>(inputBuffer);

        if (self->channels == 1) {
            std::copy(in, in + nFrames, self->internal_buffer.data());
        }
        else {
            for (unsigned int i = 0; i < nFrames; ++i) {
                float sum = 0.f;
                for (unsigned int c = 0; c < self->channels; ++c) {
                    sum += in[i * self->channels + c];
                }
                self->internal_buffer[i] = sum / self->channels;
            }
        }
        self->dsp.rt_process(self->internal_buffer);

        return 0;
    }
};