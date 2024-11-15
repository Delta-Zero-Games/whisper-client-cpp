#include "audio/audio_processor.hpp"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <stdexcept>
#include <filesystem>

namespace whisper_client {
namespace audio {

AudioProcessor::AudioProcessor()
    : QObject(nullptr)
    , ctx(nullptr)
    , modelManager(std::make_unique<ModelManager>())
    , modelLoaded(false)
{
    // Connect to model manager signals
    connect(modelManager.get(), &ModelManager::modelChanged,
            [this](const QString&) { initializeModel(); });
    
    checkModel();
}

AudioProcessor::~AudioProcessor() {
    cleanup();
}

void AudioProcessor::checkModel() {
    if (!modelManager->isModelAvailable()) {
        qWarning() << "Whisper model not found. Starting download...";
        modelManager->downloadModel(modelManager->getCurrentModel());
    } else {
        initializeModel();
    }
}

bool AudioProcessor::initializeModel() {
    cleanup();  // Clean up any existing context

    try {
        if (!modelManager->isModelAvailable()) {
            qWarning() << "Whisper model not available";
            return false;
        }

        // Initialize whisper context
        ctx = whisper_init_from_file(modelManager->getModelPath().toStdString().c_str());
        if (!ctx) {
            qWarning() << "Failed to initialize whisper context";
            return false;
        }

        modelLoaded = true;
        qDebug() << "Whisper model loaded successfully";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "Error initializing whisper model:" << e.what();
        return false;
    }
}

void AudioProcessor::cleanup() {
    if (ctx) {
        whisper_free(ctx);
        ctx = nullptr;
    }
    modelLoaded = false;
}

TranscriptionResult AudioProcessor::processAudio(const std::vector<float>& audioData) {
    if (!modelLoaded || !ctx) {
        qWarning() << "Whisper model not loaded";
        return TranscriptionResult{};
    }

    if (audioData.empty()) {
        qWarning() << "Empty audio data";
        return TranscriptionResult{};
    }

    if (onProcessingStart) {
        onProcessingStart();
    }

    TranscriptionResult result;
    try {
        result = transcribeAudio(audioData);
    } catch (const std::exception& e) {
        qWarning() << "Error processing audio:" << e.what();
    }

    if (onProcessingEnd) {
        onProcessingEnd();
    }

    return result;
}

TranscriptionResult AudioProcessor::transcribeAudio(const std::vector<float>& audioData) {
    TranscriptionResult result;

    // Initialize whisper parameters
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_special = false;
    params.print_realtime = false;
    params.print_timestamps = true;
    params.translate = false;
    params.language = language;
    params.n_threads = N_THREADS;
    params.offset_ms = 0;

    // Process the audio
    if (whisper_full(ctx, params, audioData.data(), audioData.size()) != 0) {
        qWarning() << "Failed to process audio";
        return result;
    }

    // Get number of segments
    const int n_segments = whisper_full_n_segments(ctx);
    
    // Combine all segments
    QString fullText;
    for (int i = 0; i < n_segments; ++i) {
        // Get segment text
        const char* text = whisper_full_get_segment_text(ctx, i);
        fullText += QString::fromUtf8(text).trimmed() + " ";

        // Get timing
        int64_t start = whisper_full_get_segment_t0(ctx, i);
        int64_t end = whisper_full_get_segment_t1(ctx, i);
        
        // Convert timestamps from tokens to seconds
        double start_sec = double(start) * 0.02; // whisper uses 20ms per token
        double end_sec = double(end) * 0.02;
        
        result.segments.push_back({start_sec, end_sec});
    }

    result.text = fullText.trimmed();
    result.language = QString::fromUtf8(params.language);

    // Log transcription result
    qDebug() << "Transcription completed:";
    qDebug() << "Text:" << result.text;
    qDebug() << "Language:" << result.language;
    qDebug() << "Segments:" << result.segments.size();

    return result;
}

void AudioProcessor::setProcessingStartCallback(std::function<void()> callback) {
    onProcessingStart = std::move(callback);
}

void AudioProcessor::setProcessingEndCallback(std::function<void()> callback) {
    onProcessingEnd = std::move(callback);
}

} // namespace audio
} // namespace whisper_client