#pragma once

#include <QtWebSockets/QWebSocket>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <functional>
#include <memory>
#include <optional>

namespace whisper_client {
namespace network {

class WebSocketClient : public QObject {
    Q_OBJECT

public:
    explicit WebSocketClient(QObject* parent = nullptr);
    ~WebSocketClient();

    // Connection management
    void connect(const QString& ip, const QString& port);
    void disconnect();
    bool isConnected() const;

    // Message sending
    void sendTranscript(const QString& username, const QString& text);
    void sendAction(const QString& actionType);
    void sendBotControl(bool connect);

signals:
    void connectionStatusChanged(bool connected);
    void metricsUpdated(int ttsQueue, int followers, int subs, int gifters);
    void botStatusChanged(bool connected);
    void messageReceived(const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onRequestMetrics();

private:
    void setupConnections();
    void startMetricsTimer();
    void stopMetricsTimer();
    void handleMetricsUpdate(const QJsonObject& metrics);
    void handleBotStatus(const QJsonObject& status);
    void sendMessage(const QJsonObject& message);

    std::unique_ptr<QWebSocket> socket;
    std::unique_ptr<QTimer> metricsTimer;
    QString currentUri;
    bool connected;

    // Constants
    const int RECONNECT_INTERVAL = 5000;    // 5 seconds
    const int METRICS_INTERVAL = 5000;      // 5 seconds
};

} // namespace network
} // namespace whisper_client