#include "ui/status_frame.hpp"
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>

namespace whisper_client {
namespace ui {

StatusFrame::StatusFrame(QWidget *parent)
    : QFrame(parent)
{
    setupUi();
}

StatusFrame::~StatusFrame() = default;

void StatusFrame::setupUi() {
    // Set frame style
    setFrameStyle(QFrame::Panel | QFrame::Raised);
    
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create sections
    createStatusIndicators();
    createMetricsDisplay();
}

void StatusFrame::createStatusIndicators() {
    auto* statusGroup = new QGroupBox("Status", this);
    statusLayout = new QHBoxLayout(statusGroup);

    // Create status indicators
    wsStatusDot = createStatusDot("WebSocket:");
    recordingStatusDot = createStatusDot("Recording:");
    processingStatusDot = createStatusDot("Processing:");
    
    // Create bot status with button
    auto* botContainer = new QWidget;
    auto* botLayout = new QHBoxLayout(botContainer);
    botLayout->setContentsMargins(0, 0, 0, 0);
    
    botStatusDot = createStatusDot("Bot:");
    botButton = new QPushButton("Connect Bot");
    botButton->setFixedWidth(100);
    
    botLayout->addWidget(botStatusDot);
    botLayout->addWidget(botButton);
    botLayout->addStretch();

    // Add all indicators to status layout
    statusLayout->addWidget(wsStatusDot);
    statusLayout->addWidget(recordingStatusDot);
    statusLayout->addWidget(processingStatusDot);
    statusLayout->addWidget(botContainer);
    
    mainLayout->addWidget(statusGroup);

    // Connect bot button
    connect(botButton, &QPushButton::clicked, this, &StatusFrame::onBotToggle);

    // Initialize all status indicators to inactive
    updateWebSocketStatus(false);
    updateRecordingStatus(false);
    updateProcessingStatus(false);
    updateBotStatus(false);
}

void StatusFrame::createMetricsDisplay() {
    auto* metricsGroup = new QGroupBox("Metrics", this);
    auto* metricsGrid = new QGridLayout(metricsGroup);

    // Create metric labels with their titles
    struct MetricInfo {
        QString title;
        QLabel** label;
    };

    std::vector<MetricInfo> metrics = {
        {"TTS Queue:", &ttsQueueLabel},
        {"New Followers:", &followersLabel},
        {"New Subscribers:", &subscribersLabel},
        {"New Gifters:", &giftersLabel}
    };

    int col = 0;
    for (const auto& metric : metrics) {
        auto* container = new QWidget;
        auto* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        
        layout->addWidget(new QLabel(metric.title));
        *metric.label = new QLabel("0");
        layout->addWidget(*metric.label);
        layout->addStretch();

        metricsGrid->addWidget(container, 0, col++);
    }

    mainLayout->addWidget(metricsGroup);
}

QLabel* StatusFrame::createStatusDot(const QString& labelText) {
    auto* container = new QWidget;
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    
    auto* label = new QLabel(labelText);
    auto* dot = new QLabel("⬤");
    
    layout->addWidget(label);
    layout->addWidget(dot);
    
    return dot;
}

void StatusFrame::updateStatusDot(QLabel* dot, bool active) {
    dot->setStyleSheet(QString("QLabel { color: %1; }").arg(active ? activeColor : inactiveColor));
}

void StatusFrame::updateWebSocketStatus(bool connected) {
    updateStatusDot(wsStatusDot, connected);
}

void StatusFrame::updateRecordingStatus(bool recording) {
    updateStatusDot(recordingStatusDot, recording);
}

void StatusFrame::updateProcessingStatus(bool processing) {
    updateStatusDot(processingStatusDot, processing);
}

void StatusFrame::updateBotStatus(bool connected) {
    updateStatusDot(botStatusDot, connected);
    
    if (connected) {
        botButton->setText("Disconnect Bot");
        botButton->setStyleSheet("QPushButton { background-color: #e74c3c; }"); // Red
    } else {
        botButton->setText("Connect Bot");
        botButton->setStyleSheet("QPushButton { background-color: #2ecc71; }"); // Green
    }
}

void StatusFrame::updateMetrics(int ttsQueue, int followers, int subscribers, int gifters) {
    ttsQueueLabel->setText(QString::number(ttsQueue));
    followersLabel->setText(QString::number(followers));
    subscribersLabel->setText(QString::number(subscribers));
    giftersLabel->setText(QString::number(gifters));
}

void StatusFrame::onBotToggle() {
    bool isConnected = botButton->text() == "Disconnect Bot";
    emit botToggleRequested(!isConnected);
}

} // namespace ui
} // namespace whisper_client