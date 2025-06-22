#pragma once

#include <rtaudio/RtAudio.h>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "SoundCardDrift.h"

#include <iostream>
#include "concurrentqueue.h"

#include "BandpassMixerDecim.h"




template<uint sampleRate=192000, uint block_per_sec = 1, uint decimation = 1000> 
class RtAudioCaptureThread {
public:
    static_assert(0 == sampleRate % block_per_sec);
    static_assert(0 == sampleRate %  decimation);
    static constexpr uint BLOCK_SIZE = sampleRate / block_per_sec;
    static constexpr uint OUTPUT_SIZE = BLOCK_SIZE / decimation;
    using Buffer = std::vector<std::complex<float>>;
    using BufferPtr = std::shared_ptr<Buffer>;

    std::vector<float> internal_buffer;
    RealToBasebandDecimator dsp;

    RtAudioCaptureThread(int inputDeviceId = -1,
                        uint number_channels=2,
                        size_t poolSize = 10
                        )
         :
         dsp(sampleRate,BLOCK_SIZE,OUTPUT_SIZE,decimation),
         internal_buffer(BLOCK_SIZE),
          channels(number_channels),
          inputDeviceId(inputDeviceId),
          soundcarddrift(128),
          isRunning(false),
          audio(nullptr)
    {
        for (size_t i = 0; i < poolSize; ++i) {
            auto buf = std::make_shared<Buffer>(OUTPUT_SIZE);
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
        //options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME ;
        uint bs = BLOCK_SIZE;
        audio->openStream(nullptr, &iParams, RTAUDIO_FLOAT32,
                        sampleRate, &bs, &RtAudioCaptureThread::rtCallback, this, &options);
        assert(bs==BLOCK_SIZE);

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
    int getDecimationFactor() const{
        return decimation;
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
    unsigned int channels;
    int inputDeviceId;

    std::atomic<bool> isRunning;
    std::unique_ptr<RtAudio> audio;

    moodycamel::ConcurrentQueue<BufferPtr> bufferPool;
    moodycamel::ConcurrentQueue<BufferPtr> outputQueue;
    moodycamel::ConcurrentQueue<DriftResult> driftresultqueue;


    static int rtCallback(void* outputBuffer, void* inputBuffer,
                          unsigned int nFrames, double /*streamTime*/,
                          RtAudioStreamStatus status, void* userData)
    {

        auto* self = static_cast<RtAudioCaptureThread*>(userData);
        BufferPtr buffer_output;
        
        if (status) std::cerr << "Stream underflow/overflow detected." << std::endl;
        if (!inputBuffer) return 0;
        
        self->currentFrame += nFrames;

        self->soundcarddrift.update(self->currentFrame);
        
        DriftResult driftresult;
        if(self->soundcarddrift.getResult(sampleRate, driftresult))
            self->driftresultqueue.try_enqueue(driftresult);
        if (!self->bufferPool.try_dequeue(buffer_output)) {
            return 0;
        }

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
        self->dsp.process(self->internal_buffer,buffer_output->data());
        DriftResult dr;
        if(self->soundcarddrift.getResult(self->getSampleRate(), dr)){
            std::cerr << dr << std::endl;
            self->driftresultqueue.try_enqueue(dr);
        }
        self->outputQueue.enqueue(buffer_output);
        return 0;
    }
};