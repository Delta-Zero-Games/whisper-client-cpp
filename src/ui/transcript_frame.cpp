#include "ui/transcript_frame.hpp"
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QGroupBox>

namespace whisper_client {
namespace ui {

TranscriptFrame::TranscriptFrame(QWidget *parent)
    : QFrame(parent)
{
    setupUi();
}

TranscriptFrame::~TranscriptFrame() = default;

void TranscriptFrame::setupUi() {
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Create group box
    auto* groupBox = new QGroupBox("Transcription Results:", this);
    auto* groupLayout = new QVBoxLayout(groupBox);

    // Create transcript text area
    transcriptText = new QTextEdit(this);
    transcriptText->setReadOnly(true);
    transcriptText->setLineWrapMode(QTextEdit::WidgetWidth);
    transcriptText->setMinimumHeight(200);  // Ensure reasonable default height
    
    // Set dark theme specific styling
    transcriptText->setStyleSheet(
        "QTextEdit {"
        "   background-color: #2d2d2d;"
        "   color: white;"
        "   border: 1px solid #555;"
        "}"
        "QTextEdit:focus {"
        "   border: 1px solid #666;"
        "}"
    );

    groupLayout->addWidget(transcriptText);
    mainLayout->addWidget(groupBox);
}

void TranscriptFrame::appendTranscript(const QString& username, const QString& text) {
    QString message = text.trimmed();
    if (!message.isEmpty()) {
        appendMessage(username, message, userColor);
    }
}

void TranscriptFrame::appendServerMessage(const QString& message) {
    QString trimmedMessage = message.trimmed();
    if (!trimmedMessage.isEmpty()) {
        appendMessage("Server", trimmedMessage, serverColor);
    }
}

void TranscriptFrame::appendSystemMessage(const QString& message) {
    QString trimmedMessage = message.trimmed();
    if (!trimmedMessage.isEmpty()) {
        appendMessage("System", trimmedMessage, systemColor);
    }
}

void TranscriptFrame::appendMessage(const QString& prefix, const QString& message, const QString& color) {
    // Create timestamp
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("hh:mm:ss");

    // Format message with HTML
    QString formattedMessage = QString("<span style='color: gray;'>[%1]</span> "
                                     "<span style='color: %2;'>[%3]:</span> %4")
                                     .arg(timestamp)
                                     .arg(color)
                                     .arg(prefix)
                                     .arg(message);

    // Append message
    transcriptText->append(formattedMessage);

    // Auto-scroll to bottom
    QScrollBar* scrollBar = transcriptText->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void TranscriptFrame::clear() {
    transcriptText->clear();
}

} // namespace ui
} // namespace whisper_client