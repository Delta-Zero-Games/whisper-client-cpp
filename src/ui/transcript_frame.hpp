#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QString>
#include <memory>

namespace whisper_client {
namespace ui {

class TranscriptFrame : public QFrame {
    Q_OBJECT

public:
    explicit TranscriptFrame(QWidget *parent = nullptr);
    ~TranscriptFrame();

public slots:
    void appendTranscript(const QString& username, const QString& text);
    void appendServerMessage(const QString& message);
    void appendSystemMessage(const QString& message);
    void clear();

private:
    void setupUi();
    void appendMessage(const QString& prefix, const QString& message, const QString& color = "white");

    QVBoxLayout* mainLayout;
    QTextEdit* transcriptText;

    // Color scheme for different message types
    const QString userColor = "#2ecc71";      // Green for user messages
    const QString serverColor = "#3498db";    // Blue for server messages
    const QString systemColor = "#f1c40f";    // Yellow for system messages
};

} // namespace ui
} // namespace whisper_client