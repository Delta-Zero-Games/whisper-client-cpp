#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <memory>

namespace whisper_client {

namespace network {
class WebSocketClient;
}

namespace audio {
class AudioCapture;
class AudioProcessor;
}

namespace input {
class HotkeyManager;
}

namespace ui {

class SettingsFrame;
class StatusFrame;
class TranscriptFrame;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void start();
    
    // Getter for AudioCapture - needed by SettingsFrame
    audio::AudioCapture* getAudioCapture() { return audioCapture.get(); }

public slots:
    // Status updates
    void updateWebSocketStatus(bool connected);
    void updateRecordingStatus(bool recording);
    void updateProcessingStatus(bool processing);
    void updateBotStatus(bool connected);
    void updateMetrics(int ttsQueue, int followers, int subscribers, int gifters);
    
    // Bot control
    void onBotToggleRequested(bool connect);
    
    // Transcript management
    void appendTranscript(const QString& username, const QString& text);
    void appendServerMessage(const QString& message);
    void appendSystemMessage(const QString& message);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onClosing();
    void processAudioData(const std::vector<float>& audioData);

private:
    void setupUi();
    void loadConfig();
    void saveConfig();
    void initializeComponents();
    void setupModelManager();

    // UI Components
    std::unique_ptr<SettingsFrame> settingsFrame;
    std::unique_ptr<StatusFrame> statusFrame;
    std::unique_ptr<TranscriptFrame> transcriptFrame;
    
    // Core Components
    std::unique_ptr<network::WebSocketClient> wsClient;
    std::unique_ptr<audio::AudioCapture> audioCapture;
    std::unique_ptr<audio::AudioProcessor> audioProcessor;
    std::unique_ptr<input::HotkeyManager> hotkeyManager;
    
    // UI Layout
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
};

} // namespace ui
} // namespace whisper_client