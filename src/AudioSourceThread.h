#pragma once

#include <rtaudio/RtAudio.h>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "SoundCardDrift.h"

#include <iostream>
#include "concurrentqueue.h"






class RtAudioCaptureThread {
public:
    using Buffer = std::vector<float>;
    using BufferPtr = std::shared_ptr<Buffer>;


    RtAudioCaptureThread(int inputDeviceId = -1,
                         unsigned int sampleRate = 192000,
                         unsigned int channels = 2,
                         unsigned int framesPerBuffer = 1024 * 64,
                         size_t poolSize = 10
                         )
        : sampleRate(sampleRate),
          channels(channels),
          framesPerBuffer(framesPerBuffer),
          inputDeviceId(inputDeviceId),
          soundcarddrift(128),
          isRunning(false),
          audio(nullptr)
    {
        for (size_t i = 0; i < poolSize; ++i) {
            auto buf = std::make_shared<Buffer>(framesPerBuffer);
            bufferPool.enqueue(buf);
        }
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
        options.flags = RTAUDIO_MINIMIZE_LATENCY;

        audio->openStream(nullptr, &iParams, RTAUDIO_FLOAT32,
                        sampleRate, &framesPerBuffer, &RtAudioCaptureThread::rtCallback, this, &options);

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

        BufferPtr b;
        while (bufferPool.try_dequeue(b)) {}
        while (outputQueue.try_dequeue(b)) {}
    }

    bool getBuffer(BufferPtr& p) {
        return outputQueue.try_dequeue(p);
    }
    int getSampleRate(){
        return this->sampleRate;
    }
    void returnBuffer(BufferPtr& p) {
        auto r = bufferPool.enqueue(p);
        assert(r);
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
                std::cout << oss.str() << std::endl;
                deviceList.push_back(info.name);
            }
        }
        
        return deviceList;
    }

private:
    SoundCardDrift soundcarddrift;
    unsigned int currentFrame = 0;
    unsigned int sampleRate;
    unsigned int channels;
    unsigned int framesPerBuffer;
    int inputDeviceId;

    std::atomic<bool> isRunning;
    std::unique_ptr<RtAudio> audio;

    moodycamel::ConcurrentQueue<BufferPtr> bufferPool;
    moodycamel::ConcurrentQueue<BufferPtr> outputQueue;

    static int rtCallback(void* outputBuffer, void* inputBuffer,
                          unsigned int nFrames, double /*streamTime*/,
                          RtAudioStreamStatus status, void* userData)
    {

        auto* self = static_cast<RtAudioCaptureThread*>(userData);

        
        if (status) std::cerr << "Stream underflow/overflow detected." << std::endl;
        if (!inputBuffer) return 0;
        
        self->currentFrame += nFrames;

            self->soundcarddrift.update(self->currentFrame);

            std::cerr << self->soundcarddrift.getResult(self->sampleRate) << std::endl;
        
        BufferPtr buffer;
        if (!self->bufferPool.try_dequeue(buffer)) {
            return 0;
        }

        const float* in = static_cast<const float*>(inputBuffer);

        if (self->channels == 1) {
            std::copy(in, in + nFrames, buffer->begin());
        }
        else {
            for (unsigned int i = 0; i < nFrames; ++i) {
                float sum = 0.f;
                for (unsigned int c = 0; c < self->channels; ++c) {
                    sum += in[i * self->channels + c];
                }
                (*buffer)[i] = sum / self->channels;
            }
        }
        self->outputQueue.enqueue(buffer);
        return 0;
    }
};