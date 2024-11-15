#include "ui/main_window.hpp"
#include "ui/settings_frame.hpp"
#include "ui/status_frame.hpp"
#include "ui/transcript_frame.hpp"
#include "network/websocket_client.hpp"
#include "audio/audio_capture.hpp"
#include "audio/audio_processor.hpp"
#include "audio/model_manager.hpp"
#include "input/hotkey_manager.hpp"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtGui/QCloseEvent>
#include <QtGui/QIcon>
#include <QtCore/QDebug>

namespace whisper_client {
namespace ui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settingsFrame(std::make_unique<SettingsFrame>(this))
    , statusFrame(std::make_unique<StatusFrame>(this))
    , transcriptFrame(std::make_unique<TranscriptFrame>(this))
    , wsClient(std::make_unique<network::WebSocketClient>(this))
    , audioCapture(std::make_unique<audio::AudioCapture>())
    , audioProcessor(std::make_unique<audio::AudioProcessor>())
    , hotkeyManager(std::make_unique<input::HotkeyManager>(this))
{
    setupUi();
    loadConfig();
    initializeComponents();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("Whisper Client");
    resize(600, 750);

    // Set window icon
    QIcon icon(":/icons/dzp.ico");
    setWindowIcon(icon);

    // Create central widget and main layout
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Add frames to the layout
    mainLayout->addWidget(settingsFrame.get());
    mainLayout->addWidget(statusFrame.get());
    mainLayout->addWidget(transcriptFrame.get());
    mainLayout->setStretchFactor(transcriptFrame.get(), 1);  // Give transcript frame extra space

    // Set dark theme
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    QApplication::setPalette(darkPalette);
    QApplication::setStyle("Fusion");
}

void MainWindow::loadConfig() {
    appendSystemMessage("Loading configuration...");
}

void MainWindow::saveConfig() {
    settingsFrame->saveSettings();
    appendSystemMessage("Configuration saved.");
}

void MainWindow::initializeComponents() {
    appendSystemMessage("Initializing components...");

    // Set up model manager connections
    setupModelManager();

    // Initialize audio capture callbacks
    audioCapture->setRecordingStartCallback([this]() {
        updateRecordingStatus(true);
    });
    
    audioCapture->setRecordingStopCallback([this]() {
        updateRecordingStatus(false);
    });

    // Initialize audio processor callbacks
    audioProcessor->setProcessingStartCallback([this]() {
        updateProcessingStatus(true);
    });
    
    audioProcessor->setProcessingEndCallback([this]() {
        updateProcessingStatus(false);
    });

    // Initialize hotkey manager
    connect(hotkeyManager.get(), &input::HotkeyManager::recordingStarted,
            [this]() {
                audioCapture->startRecording();
            });
            
    connect(hotkeyManager.get(), &input::HotkeyManager::recordingStopped,
            [this]() {
                if (audioCapture->isRecording()) {
                    auto audioData = audioCapture->stopRecording();
                    if (!audioData.empty()) {
                        processAudioData(audioData);
                    }
                }
            });
            
    connect(hotkeyManager.get(), &input::HotkeyManager::actionTriggered,
            [this](const QString& action) {
                if (wsClient && wsClient->isConnected()) {
                    wsClient->sendAction(action);
                    appendSystemMessage(QString("Action triggered: %1").arg(action));
                }
            });

    // Initialize WebSocket connections
    connect(wsClient.get(), &network::WebSocketClient::connectionStatusChanged,
            this, &MainWindow::updateWebSocketStatus);
    connect(wsClient.get(), &network::WebSocketClient::messageReceived,
            this, &MainWindow::appendServerMessage);
    connect(wsClient.get(), &network::WebSocketClient::metricsUpdated,
            this, &MainWindow::updateMetrics);
    connect(wsClient.get(), &network::WebSocketClient::botStatusChanged,
            this, &MainWindow::updateBotStatus);

    // Connect status frame signals
    connect(statusFrame.get(), &StatusFrame::botToggleRequested,
            this, &MainWindow::onBotToggleRequested);

    // Set initial hotkeys from settings
    hotkeyManager->setRecordingHotkey(settingsFrame->getPushToTalkKey());
    hotkeyManager->setRecordingMode(settingsFrame->isToggleModeEnabled() ? "toggle" : "push");
    
    // Set action hotkeys
    const QStringList actions = {"tts", "follows", "subs", "gifts"};
    for (const QString& action : actions) {
        QString hotkey = settingsFrame->getActionHotkey(action);
        if (!hotkey.isEmpty()) {
            hotkeyManager->setActionHotkey(action, hotkey);
        }
    }

    // Start hotkey manager
    if (!hotkeyManager->start()) {
        appendSystemMessage("Failed to start hotkey manager");
    }

    // Initialize with disconnected state
    updateWebSocketStatus(false);
    updateRecordingStatus(false);
    updateProcessingStatus(false);
    updateBotStatus(false);
    updateMetrics(0, 0, 0, 0);

    appendSystemMessage("Initialization complete.");
}

void MainWindow::setupModelManager() {
    auto modelManager = audioProcessor->getModelManager();
    if (modelManager) {
        connect(modelManager, &audio::ModelManager::downloadProgress,
                [this](qint64 received, qint64 total) {
                    QString progress = QString("Downloading model: %1%")
                        .arg((received * 100) / total);
                    appendSystemMessage(progress);
                });

        connect(modelManager, &audio::ModelManager::downloadComplete,
                [this](bool success, const QString& message) {
                    appendSystemMessage(QString("Model download %1: %2")
                        .arg(success ? "completed" : "failed")
                        .arg(message));
                });
    }
}

void MainWindow::processAudioData(const std::vector<float>& audioData) {
    if (audioData.empty()) {
        return;
    }

    // Process the audio and get transcription
    audio::TranscriptionResult result = audioProcessor->processAudio(audioData);
    
    if (!result.text.isEmpty()) {
        // Send transcript to WebSocket if connected
        if (wsClient && wsClient->isConnected()) {
            wsClient->sendTranscript(
                settingsFrame->getSelectedUser(),
                result.text
            );
        }
        
        // Display transcript
        appendTranscript(
            settingsFrame->getSelectedUser(),
            result.text
        );
    }
}

void MainWindow::updateWebSocketStatus(bool connected) {
    statusFrame->updateWebSocketStatus(connected);
    appendSystemMessage(connected ? "WebSocket connected." : "WebSocket disconnected.");
}

void MainWindow::updateRecordingStatus(bool recording) {
    statusFrame->updateRecordingStatus(recording);
    if (recording) {
        appendSystemMessage("Recording started.");
    }
}

void MainWindow::updateProcessingStatus(bool processing) {
    statusFrame->updateProcessingStatus(processing);
    if (processing) {
        appendSystemMessage("Processing audio...");
    }
}

void MainWindow::updateBotStatus(bool connected) {
    statusFrame->updateBotStatus(connected);
    appendSystemMessage(connected ? "Bot connected." : "Bot disconnected.");
}

void MainWindow::updateMetrics(int ttsQueue, int followers, int subscribers, int gifters) {
    statusFrame->updateMetrics(ttsQueue, followers, subscribers, gifters);
}

void MainWindow::onBotToggleRequested(bool connect) {
    if (wsClient->isConnected()) {
        wsClient->sendBotControl(connect);
        appendSystemMessage(connect ? "Connecting bot..." : "Disconnecting bot...");
    } else {
        appendSystemMessage("Cannot control bot: WebSocket not connected");
    }
}

void MainWindow::appendTranscript(const QString& username, const QString& text) {
    transcriptFrame->appendTranscript(username, text);
}

void MainWindow::appendServerMessage(const QString& message) {
    transcriptFrame->appendServerMessage(message);
}

void MainWindow::appendSystemMessage(const QString& message) {
    transcriptFrame->appendSystemMessage(message);
}

void MainWindow::start() {
    show();
    appendSystemMessage("Application started.");

    // Start WebSocket if enabled
    if (settingsFrame->isWebSocketEnabled()) {
        appendSystemMessage("Connecting to WebSocket server...");
        wsClient->connect(
            settingsFrame->getWebSocketIP(),
            settingsFrame->getWebSocketPort()
        );
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    onClosing();
    event->accept();
}

void MainWindow::onClosing() {
    try {
        appendSystemMessage("Shutting down...");
        
        // Stop hotkey manager
        if (hotkeyManager) {
            hotkeyManager->stop();
        }

        // Stop recording if active
        if (audioCapture && audioCapture->isRecording()) {
            audioCapture->stopRecording();
        }

        // Clean up audio processor
        if (audioProcessor) {
            audioProcessor->cleanup();
        }

        // Disconnect WebSocket
        if (wsClient) {
            wsClient->disconnect();
        }

        // Save configuration
        saveConfig();

    } catch (const std::exception& e) {
        QMessageBox::warning(
            this,
            "Error",
            QString("Error during shutdown: %1").arg(e.what())
        );
    }
}

} // namespace ui
} // namespace whisper_client