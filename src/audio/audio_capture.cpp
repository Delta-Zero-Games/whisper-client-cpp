#include "audio/audio_capture.hpp"
#include <QtCore/QDebug>
#include <algorithm>

namespace whisper_client {
namespace audio {

AudioCapture::AudioCapture()
    : audio(std::make_unique<RtAudio>())
    , currentDeviceId(0)
    , recording(false)
{
    // Try to find a default input device
    try {
        auto devices = listInputDevices();
        for (const auto& device : devices) {
            if (device.isDefault) {
                currentDeviceId = device.id;
                break;
            }
        }
    }
    catch (const RtAudioError& e) {
        qWarning() << "RtAudio error:" << e.getMessage().c_str();
    }
}

AudioCapture::~AudioCapture() {
    if (recording) {
        stopRecording();
    }
}

std::vector<AudioDevice> AudioCapture::listInputDevices() {
    std::vector<AudioDevice> devices;
    
    try {
        // Get the default input device ID
        unsigned int defaultId = audio->getDefaultInputDevice();
        
        // Probe all devices
        RtAudio::DeviceInfo info;
        unsigned int deviceCount = audio->getDeviceCount();
        
        for (unsigned int i = 0; i < deviceCount; i++) {
            info = audio->getDeviceInfo(i);
            
            // Only add devices with input channels
            if (info.inputChannels > 0) {
                AudioDevice device;
                device.id = i;
                device.name = QString::fromStdString(info.name);
                device.channels = info.inputChannels;
                device.isDefault = (i == defaultId);
                
                devices.push_back(device);
            }
        }
    }
    catch (const RtAudioError& e) {
        qWarning() << "Error listing audio devices:" << e.getMessage().c_str();
    }
    
    return devices;
}

bool AudioCapture::setDevice(unsigned int deviceId) {
    try {
        // Stop any active recording
        if (recording) {
            stopRecording();
        }
        
        // Verify device exists and has input channels
        RtAudio::DeviceInfo info = audio->getDeviceInfo(deviceId);
        if (info.inputChannels == 0) {
            qWarning() << "Selected device has no input channels";
            return false;
        }
        
        currentDeviceId = deviceId;
        qDebug() << "Audio device set to:" << QString::fromStdString(info.name);
        return true;
    }
    catch (const RtAudioError& e) {
        qWarning() << "Error setting audio device:" << e.getMessage().c_str();
        return false;
    }
}

unsigned int AudioCapture::getCurrentDevice() const {
    return currentDeviceId;
}

bool AudioCapture::startRecording() {
    if (recording) {
        return true;
    }

    try {
        // Clear any existing audio data
        clearBuffer();
        
        // Set up the stream parameters
        RtAudio::StreamParameters params;
        params.deviceId = currentDeviceId;
        params.nChannels = channels;
        params.firstChannel = 0;
        
        // Open the stream
        audio->openStream(
            nullptr,      // No output
            &params,      // Input parameters
            RTAUDIO_FLOAT32,  // Using float samples
            sampleRate,
            &bufferFrames,
            &AudioCapture::recordCallback,
            this
        );
        
        // Start the stream
        audio->startStream();
        recording = true;
        
        if (onRecordingStart) {
            onRecordingStart();
        }
        
        qDebug() << "Recording started";
        return true;
    }
    catch (const RtAudioError& e) {
        qWarning() << "Error starting recording:" << e.getMessage().c_str();
        recording = false;
        return false;
    }
}

std::vector<float> AudioCapture::stopRecording() {
    if (!recording) {
        return std::vector<float>();
    }

    try {
        // Stop the stream
        audio->stopStream();
        if (audio->isStreamOpen()) {
            audio->closeStream();
        }
        
        recording = false;
        
        // Combine all audio data
        std::vector<float> combinedAudio;
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            while (!audioQueue.empty()) {
                auto& chunk = audioQueue.front();
                combinedAudio.insert(combinedAudio.end(), chunk.begin(), chunk.end());
                audioQueue.pop();
            }
        }
        
        if (onRecordingStop) {
            onRecordingStop();
        }
        
        qDebug() << "Recording stopped, collected" << combinedAudio.size() << "samples";
        return combinedAudio;
    }
    catch (const RtAudioError& e) {
        qWarning() << "Error stopping recording:" << e.getMessage().c_str();
        recording = false;
        return std::vector<float>();
    }
}

bool AudioCapture::isRecording() const {
    return recording;
}

void AudioCapture::setRecordingStartCallback(std::function<void()> callback) {
    onRecordingStart = std::move(callback);
}

void AudioCapture::setRecordingStopCallback(std::function<void()> callback) {
    onRecordingStop = std::move(callback);
}

int AudioCapture::recordCallback(void* outputBuffer, void* inputBuffer,
                               unsigned int nFrames, double streamTime,
                               RtAudioStreamStatus status, void* userData) {
    (void)outputBuffer;  // Unused
    (void)streamTime;    // Unused
    
    if (status) {
        qWarning() << "Stream overflow detected!";
    }
    
    auto* capture = static_cast<AudioCapture*>(userData);
    const float* input = static_cast<const float*>(inputBuffer);
    
    capture->processAudioData(input, nFrames);
    
    return 0;
}

void AudioCapture::processAudioData(const float* buffer, unsigned int frames) {
    std::vector<float> chunk(buffer, buffer + frames * channels);
    
    std::lock_guard<std::mutex> lock(audioMutex);
    audioQueue.push(std::move(chunk));
}

void AudioCapture::clearBuffer() {
    std::lock_guard<std::mutex> lock(audioMutex);
    std::queue<std::vector<float>> empty;
    std::swap(audioQueue, empty);
}

} // namespace audio
} // namespace whisper_client