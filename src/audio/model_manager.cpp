#include "audio/model_manager.hpp"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>

namespace whisper_client {
namespace audio {

ModelManager::ModelManager(QObject* parent)
    : QObject(parent)
    , networkManager(std::make_unique<QNetworkAccessManager>())
    , currentDownload(nullptr)
{
    // Set up model directory
    QString appDir = QCoreApplication::applicationDirPath();
    modelDir = appDir + "/models";
    createModelDirectory();
    
    // Initialize model information
    initializeModelInfo();
    
    // Set default model
    currentModelName = DEFAULT_MODEL;
}

ModelManager::~ModelManager() {
    cancelDownload();
}

void ModelManager::initializeModelInfo() {
    // Initialize model information with URLs and file sizes
    modelInfos = {
        {"tiny", {
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin",
            75_000_000,
            "be07e048e1e599ad46341c8d2a135645097a538221678b7acdd1b1919c6e1d21"
        }},
        {"base", {
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin",
            142_000_000,
            "137c40403d78fd54d454da0f9bd998f78703edb8f75bc929ad2a6cf1fa7d519b"
        }},
        {"small", {
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin",
            466_000_000,
            "55356645c8b389a8277d7d82f084116a9d2b8c6d365c3b6ad3eb3a7dc8303c89"
        }},
        {"medium", {
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin",
            1_500_000_000,
            "fd9727b6e1217c2f614f9b698455c4ffd82463b25a7d1822a6ba8e9c80824d52"
        }},
        {"large", {
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large.bin",
            2_900_000_000,
            "0f4c8e34f21cf1a914c59d8b3ce882345ad349d86b9ecd5646f579659c661257"
        }}
    };
}

void ModelManager::createModelDirectory() {
    QDir dir;
    if (!dir.exists(modelDir)) {
        dir.mkpath(modelDir);
        qDebug() << "Created model directory:" << modelDir;
    }
}

bool ModelManager::isModelAvailable() const {
    return verifyModelFile(getModelPath(currentModelName));
}

QString ModelManager::getModelPath() const {
    return getModelPath(currentModelName);
}

QString ModelManager::getCurrentModel() const {
    return currentModelName;
}

QStringList ModelManager::getAvailableModels() const {
    return AVAILABLE_MODELS;
}

bool ModelManager::setModel(const QString& modelName) {
    if (!AVAILABLE_MODELS.contains(modelName)) {
        qWarning() << "Invalid model name:" << modelName;
        return false;
    }

    currentModelName = modelName;
    emit modelChanged(modelName);
    return true;
}

void ModelManager::downloadModel(const QString& modelName) {
    if (!AVAILABLE_MODELS.contains(modelName)) {
        emit downloadComplete(false, "Invalid model name");
        return;
    }

    if (currentDownload) {
        emit downloadComplete(false, "Download already in progress");
        return;
    }

    QString url = getModelUrl(modelName);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);

    currentDownload = networkManager->get(request);

    connect(currentDownload, &QNetworkReply::downloadProgress,
            this, &ModelManager::onDownloadProgress);
    connect(currentDownload, &QNetworkReply::finished,
            this, &ModelManager::onDownloadFinished);
    connect(currentDownload, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &ModelManager::onDownloadError);

    qDebug() << "Starting download of model:" << modelName;
}

void ModelManager::cancelDownload() {
    if (currentDownload) {
        currentDownload->abort();
        currentDownload->deleteLater();
        currentDownload = nullptr;
        qDebug() << "Download cancelled";
    }
}

void ModelManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void ModelManager::onDownloadFinished() {
    if (!currentDownload) return;

    if (currentDownload->error() == QNetworkReply::NoError) {
        QString modelPath = getModelPath(currentModelName);
        QFile file(modelPath);
        
        if (file.open(QIODevice::WriteOnly)) {
            file.write(currentDownload->readAll());
            file.close();

            // Verify the downloaded file
            if (verifyModelFile(modelPath)) {
                emit downloadComplete(true, "Download completed successfully");
            } else {
                file.remove();
                emit downloadComplete(false, "Model verification failed");
            }
        } else {
            emit downloadComplete(false, "Failed to save model file");
        }
    }

    currentDownload->deleteLater();
    currentDownload = nullptr;
}

void ModelManager::onDownloadError(QNetworkReply::NetworkError error) {
    QString errorMessage = currentDownload ? currentDownload->errorString() 
                                         : "Unknown error";
    emit downloadComplete(false, errorMessage);
    
    if (currentDownload) {
        currentDownload->deleteLater();
        currentDownload = nullptr;
    }
}

QString ModelManager::getModelUrl(const QString& modelName) const {
    return modelInfos.value(modelName).url;
}

QString ModelManager::getModelPath(const QString& modelName) const {
    return QString("%1/ggml-%2.bin").arg(modelDir, modelName);
}

bool ModelManager::verifyModelFile(const QString& modelPath) const {
    QFile file(modelPath);
    if (!file.exists()) return false;
    
    // Verify file size
    qint64 expectedSize = modelInfos.value(currentModelName).size;
    if (file.size() != expectedSize) {
        qWarning() << "Model file size mismatch. Expected:" << expectedSize 
                   << "Got:" << file.size();
        return false;
    }

    // Optionally verify hash
    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) {
            QString fileHash = hash.result().toHex();
            QString expectedHash = modelInfos.value(currentModelName).hash;
            if (fileHash != expectedHash) {
                qWarning() << "Model file hash mismatch";
                return false;
            }
        }
        file.close();
    }

    return true;
}

} // namespace audio
} // namespace whisper_client