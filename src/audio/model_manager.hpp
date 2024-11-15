#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <memory>

namespace whisper_client {
namespace audio {

class ModelManager : public QObject {
    Q_OBJECT

public:
    explicit ModelManager(QObject* parent = nullptr);
    ~ModelManager();

    // Model management
    bool isModelAvailable() const;
    QString getModelPath() const;
    QString getCurrentModel() const;
    
    // Available models
    QStringList getAvailableModels() const;
    bool setModel(const QString& modelName);

public slots:
    void downloadModel(const QString& modelName);
    void cancelDownload();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadComplete(bool success, const QString& message);
    void modelChanged(const QString& modelName);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);

private:
    void initializeModelInfo();
    QString getModelUrl(const QString& modelName) const;
    QString getModelPath(const QString& modelName) const;
    bool verifyModelFile(const QString& modelPath) const;
    void createModelDirectory();

    std::unique_ptr<QNetworkAccessManager> networkManager;
    QNetworkReply* currentDownload;
    QString modelDir;
    QString currentModelName;
    
    struct ModelInfo {
        QString url;
        qint64 size;
        QString hash;
    };
    QMap<QString, ModelInfo> modelInfos;

    // Constants
    const QString DEFAULT_MODEL = "base";
    const QStringList AVAILABLE_MODELS = {
        "tiny", "base", "small", "medium", "large"
    };
};

} // namespace audio
} // namespace whisper_client