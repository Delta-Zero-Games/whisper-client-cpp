#include "network/websocket_client.hpp"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

namespace whisper_client {
namespace network {

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent)
    , socket(std::make_unique<QWebSocket>())
    , metricsTimer(std::make_unique<QTimer>())
    , connected(false)
{
    setupConnections();
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::setupConnections() {
    // Socket connections
    QObject::connect(socket.get(), &QWebSocket::connected,
                    this, &WebSocketClient::onConnected);
    QObject::connect(socket.get(), &QWebSocket::disconnected,
                    this, &WebSocketClient::onDisconnected);
    QObject::connect(socket.get(), &QWebSocket::textMessageReceived,
                    this, &WebSocketClient::onTextMessageReceived);
    QObject::connect(socket.get(), 
                    QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                    this, &WebSocketClient::onError);

    // Metrics timer connection
    QObject::connect(metricsTimer.get(), &QTimer::timeout,
                    this, &WebSocketClient::onRequestMetrics);
}

void WebSocketClient::connect(const QString& ip, const QString& port) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        return;
    }

    currentUri = QString("ws://%1:%2").arg(ip, port);
    qDebug() << "Connecting to:" << currentUri;
    
    socket->open(QUrl(currentUri));
}

void WebSocketClient::disconnect() {
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->close();
    }
    stopMetricsTimer();
}

bool WebSocketClient::isConnected() const {
    return connected;
}

void WebSocketClient::onConnected() {
    qDebug() << "WebSocket connected";
    connected = true;
    emit connectionStatusChanged(true);
    startMetricsTimer();

    // Send initial connection message
    QJsonObject message;
    message["type"] = "connect";
    sendMessage(message);
}

void WebSocketClient::onDisconnected() {
    qDebug() << "WebSocket disconnected";
    connected = false;
    emit connectionStatusChanged(false);
    stopMetricsTimer();

    // Reset metrics
    emit metricsUpdated(0, 0, 0, 0);
}

void WebSocketClient::onTextMessageReceived(const QString& message) {
    try {
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        if (!doc.isObject()) {
            qWarning() << "Received invalid JSON message";
            return;
        }

        QJsonObject obj = doc.object();
        QString messageType = obj["type"].toString();

        if (messageType == "metrics_update") {
            handleMetricsUpdate(obj["metrics"].toObject());
        }
        else if (messageType == "bot_status") {
            handleBotStatus(obj);
        }
        else {
            emit messageReceived(message);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Error processing message:" << e.what();
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error) {
    qWarning() << "WebSocket error:" << error << "-" << socket->errorString();
    emit messageReceived(QString("WebSocket error: %1").arg(socket->errorString()));
}

void WebSocketClient::sendTranscript(const QString& username, const QString& text) {
    if (!connected) return;

    QJsonObject message;
    message["type"] = "transcript";
    message["username"] = username;
    message["content"] = text;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    sendMessage(message);
}

void WebSocketClient::sendAction(const QString& actionType) {
    if (!connected) return;

    QJsonObject message;
    message["type"] = actionType;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    sendMessage(message);
}

void WebSocketClient::sendBotControl(bool connect) {
    if (!connected) return;

    QJsonObject message;
    message["type"] = "bot_control";
    message["action"] = connect ? "connect" : "disconnect";

    sendMessage(message);
}

void WebSocketClient::startMetricsTimer() {
    metricsTimer->start(METRICS_INTERVAL);
}

void WebSocketClient::stopMetricsTimer() {
    metricsTimer->stop();
}

void WebSocketClient::onRequestMetrics() {
    if (!connected) return;

    QJsonObject message;
    message["type"] = "request_metrics";
    sendMessage(message);
}

void WebSocketClient::handleMetricsUpdate(const QJsonObject& metrics) {
    emit metricsUpdated(
        metrics["tts_in_queue"].toInt(0),
        metrics["new_followers_count"].toInt(0),
        metrics["new_subs_count"].toInt(0),
        metrics["new_giver_count"].toInt(0)
    );
}

void WebSocketClient::handleBotStatus(const QJsonObject& status) {
    bool botConnected = status["connected"].toBool(false);
    emit botStatusChanged(botConnected);
}

void WebSocketClient::sendMessage(const QJsonObject& message) {
    if (!connected) return;

    QJsonDocument doc(message);
    QString strMessage = doc.toJson(QJsonDocument::Compact);
    socket->sendTextMessage(strMessage);
}

} // namespace network
} // namespace whisper_client