#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <memory>

namespace whisper_client {
namespace ui {

class StatusFrame : public QFrame {
    Q_OBJECT

public:
    explicit StatusFrame(QWidget *parent = nullptr);
    ~StatusFrame();

public slots:
    void updateWebSocketStatus(bool connected);
    void updateRecordingStatus(bool recording);
    void updateProcessingStatus(bool processing);
    void updateBotStatus(bool connected);
    void updateMetrics(int ttsQueue, int followers, int subscribers, int gifters);

private slots:
    void onBotToggle();

signals:
    void botToggleRequested(bool connect);

private:
    void setupUi();
    void createStatusIndicators();
    void createMetricsDisplay();
    QLabel* createStatusDot(const QString& labelText);
    void updateStatusDot(QLabel* dot, bool active);

    // Status indicators
    QLabel* wsStatusDot;
    QLabel* recordingStatusDot;
    QLabel* processingStatusDot;
    QLabel* botStatusDot;
    QPushButton* botButton;

    // Metrics labels
    QLabel* ttsQueueLabel;
    QLabel* followersLabel;
    QLabel* subscribersLabel;
    QLabel* giftersLabel;

    // Layouts
    QVBoxLayout* mainLayout;
    QHBoxLayout* statusLayout;
    QHBoxLayout* metricsLayout;

    // Colors
    const QString activeColor = "#2ecc71";    // Green
    const QString inactiveColor = "#e74c3c";  // Red
};

} // namespace ui
} // namespace whisper_client