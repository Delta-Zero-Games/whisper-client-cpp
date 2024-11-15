#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <QtCore/QString>
#include "whisper.h"
#include "audio/model_manager.hpp"

namespace whisper_client {
namespace audio {

struct TranscriptionResult {
    QString text;
    QString language;
    std::vector<std::pair<double, double>> segments;  // start_time, end_time pairs
};

class AudioProcessor : public QObject {
    Q_OBJECT

public:
    AudioProcessor();
    ~AudioProcessor();

    // Processing control
    TranscriptionResult processAudio(const std::vector<float>& audioData);
    void cleanup();

    // Model management
    ModelManager* getModelManager() { return modelManager.get(); }

    // Callbacks
    void setProcessingStartCallback(std::function<void()> callback);
    void setProcessingEndCallback(std::function<void()> callback);

private slots:
    bool initializeModel();
    void checkModel();

private:
    TranscriptionResult transcribeAudio(const std::vector<float>& audioData);
    
    struct whisper_context* ctx;
    std::unique_ptr<ModelManager> modelManager;

    // Processing settings
    const int WHISPER_SAMPLE_RATE = 16000;
    const int N_THREADS = 4;         // Number of processing threads
    const char* language = "en";     // Default language
    
    // Callbacks
    std::function<void()> onProcessingStart;
    std::function<void()> onProcessingEnd;
    
    // Internal state
    bool modelLoaded;
};

} // namespace audio
} // namespace whisper_client