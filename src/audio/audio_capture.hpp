#pragma once

#include <RtAudio.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <functional>
#include <QtCore/QString>
#include <stdexcept>

namespace whisper_client {
namespace audio {

struct AudioDevice {
    unsigned int id;
    QString name;
    unsigned int channels;
    bool isDefault;
};

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    // Device management
    std::vector<AudioDevice> listInputDevices();
    bool setDevice(unsigned int deviceId);
    unsigned int getCurrentDevice() const;

    // Recording control
    bool startRecording();
    std::vector<float> stopRecording();
    bool isRecording() const;

    // Callbacks
    void setRecordingStartCallback(std::function<void()> callback);
    void setRecordingStopCallback(std::function<void()> callback);

private:
    static int recordCallback(void* outputBuffer, void* inputBuffer,
                            unsigned int nFrames, double streamTime,
                            RtAudioStreamStatus status, void* userData);

    void processAudioData(const float* buffer, unsigned int frames);
    void clearBuffer();

    std::unique_ptr<RtAudio> audio;
    std::queue<std::vector<float>> audioQueue;
    std::mutex audioMutex;
    
    unsigned int currentDeviceId;
    bool recording;
    
    // Audio settings
    const unsigned int sampleRate = 16000;  // Required for Whisper
    const unsigned int channels = 1;        // Mono recording
    const unsigned int bufferFrames = 1024; // Buffer size
    
    // Callbacks
    std::function<void()> onRecordingStart;
    std::function<void()> onRecordingStop;
};

} // namespace audio
} // namespace whisper_client