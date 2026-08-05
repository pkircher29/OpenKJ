#include "songshop.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QUrlQuery>


SongShop::SongShop(QObject *parent) : QObject(parent) {
    m_logger = spdlog::get("logger");
    connectionReset = false;
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::sslErrors, this, &SongShop::onSslErrors);
    connect(manager, &QNetworkAccessManager::finished, this, &SongShop::onNetworkReply);
    songsLoaded = false;
    knLoginError = false;
}

void SongShop::updateCache() {
    // return;
    m_logger->info("{} Requesting songs from db.openkj.org", m_loggingPrefix);
    QJsonObject mainObject;
    mainObject.insert("command", "getsongs");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl("https://db.openkj.org/apigetsongs_v2"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto reply = manager->post(request, jsonDocument.toJson());
    reply->setProperty("requestType", "catalog");
}

ShopSongs SongShop::getSongs() {
    if (!songsLoaded)
        updateCache();
    return songs;
}

void SongShop::knLogin(QString userName, QString password) {
    knLoginError = false;
    const QString passHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex();
    QUrl url("https://www.partytyme.net/songshop/cat/api_account_setup.php");
    QUrlQuery query;
    query.addQueryItem("action", "validate_login");
    query.addQueryItem("username", userName);
    query.addQueryItem("md5", passHash);
    query.addQueryItem("merchant", "99");
    url.setQuery(query);
    QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30000);
#endif
    auto reply = manager->get(request);
    reply->setProperty("requestType", "login");
}

void SongShop::knPurchase(QString songId, QString ccNumber, QString ccM, QString ccY, QString ccCVV) {
    QUrl url("https://www.partytyme.net/songshop/cat/api_make_order.php");
    QUrlQuery query;
    query.addQueryItem("media_format", songId.contains("PY") ? "mp3g" : "mp4");
    query.addQueryItem("tracks", songId);
    query.addQueryItem("cc_number", ccNumber);
    query.addQueryItem("cc_cvv", ccCVV);
    query.addQueryItem("cc_exp_mm", ccM);
    query.addQueryItem("cc_exp_yyyy", ccY);
    query.addQueryItem("session_id", knSessionId);
    query.addQueryItem("merchant", "99");
    url.setQuery(query);
    QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30000);
#endif
    auto reply = manager->get(request);
    reply->setProperty("requestType", "purchase");
}

bool SongShop::loggedIn() {
    if (knSessionId != "")
        return true;
    return false;
}

void SongShop::setDlSongInfo(QString artist, QString title, QString songId) {
    dlArtist = artist;
    dlTitle = title;
    dlSongId = songId;
}

void SongShop::downloadFile(const QString &url, const QString &destFn) {
    QString destDir = m_settings.storeDownloadDir();
    if (!QDir().mkpath(destDir)) {
        m_logger->error("{} Could not create download directory: {}", m_loggingPrefix, destDir);
        emit paymentProcessingFailed();
        return;
    }
    const QString safeFileName = QFileInfo(destFn).fileName();
    if (safeFileName.isEmpty()) {
        m_logger->error("{} Refusing download with an empty destination filename", m_loggingPrefix);
        emit paymentProcessingFailed();
        return;
    }
    const QString destPath = QDir(destDir).filePath(safeFileName);
    QNetworkAccessManager m_NetworkMngr;
    QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(120000);
#endif
    QNetworkReply *reply = m_NetworkMngr.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::downloadProgress, this, &SongShop::onDownloadProgress);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        m_logger->error("{} Song download failed: {}", m_loggingPrefix, reply->errorString());
        delete reply;
        emit paymentProcessingFailed();
        return;
    }
    const QByteArray downloadData = reply->readAll();
    delete reply;
    QSaveFile file(destPath);
    if (!file.open(QIODevice::WriteOnly) || file.write(downloadData) != downloadData.size() || !file.commit()) {
        m_logger->error("{} Could not save downloaded song to: {}", m_loggingPrefix, destPath);
        emit paymentProcessingFailed();
        return;
    }
    emit karaokeSongDownloaded(destPath);
    // clear session ID to force login again before next download.  Workaround for expiring PartyTyme logins.
    knSessionId = "";
}

void SongShop::onSslErrors(QNetworkReply *reply, QList<QSslError> errors) {
    reply->abort();
    m_logger->warn("{} Received SSL error when connecting to db.openkj.org, please make sure your system time and date are correct", m_loggingPrefix);
}

void SongShop::onNetworkReply(QNetworkReply *reply) {
    m_logger->trace("{} Received network reply from db.openkj.org", m_loggingPrefix);
    const QString requestType = reply->property("requestType").toString();
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        m_logger->warn("{} Error connecting to server: {}", m_loggingPrefix, reply->errorString());
        if (requestType == "login") {
            knLoginError = true;
            knSessionId.clear();
            emit knLoginFailure();
        } else if (requestType == "purchase") {
            emit paymentProcessingFailed();
        }
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    if (error) {
        m_logger->warn("{} Received error reply from server: {}", m_loggingPrefix, json.object().value("errorString").toString());
        if (requestType == "login") {
            knLoginError = true;
            knSessionId.clear();
            emit knLoginFailure();
        } else if (requestType == "purchase") {
            emit paymentProcessingFailed();
        }
        return;
    }
    if (command == "getsongs") {
        emit songUpdateStarted();
        QJsonArray songsArray = json.object().value("songs").toArray();
        songs.clear();
        for (int i = 0; i < songsArray.size(); i++) {
            ShopSong song;
            QJsonObject jsonObject = songsArray.at(i).toObject();
            song.artist = jsonObject.value("artist").toString();
            song.title = jsonObject.value("title").toString();
            song.songid = jsonObject.value("songid").toString();
            song.vendor = jsonObject.value("vendor").toString();
            song.price = jsonObject.value("price").toDouble();
            song.type = 0;
            songs.append(song);
        }
        if (!songs.isEmpty())
            songsLoaded = true;
        emit songsUpdated();
    } else if ((json.object().value("result").toString() == "SUCCESS") &&
               (json.object().value("session_id").toString() != "")) {
        knSessionId = json.object().value("session_id").toString();
        knLoginError = false;
        emit knLoginSuccess();
    } else if ((json.object().value("result").toString() == "ERROR") &&
               (json.object().value("error").toString() == "Wrong username or password")) {
        m_logger->warn("PartyTyme login failed, incorrect username or password", m_loggingPrefix);
        knLoginError = true;
        knSessionId = "";
        emit knLoginFailure();
    } else if ((json.object().value("result").toString() == "ERROR") &&
               (json.object().value("error").toString() == "User not logged in")) {
        m_logger->warn("{} PartyTyme reported user not logged in", m_loggingPrefix);
        knSessionId = "";
    } else if ((json.object().value("result").toString() == "SUCCESS") &&
               (json.object().value("download_links").isArray())) {
        m_logger->info("{} Purchase successful", m_loggingPrefix);
        QString url = json.object().value("download_links").toArray().at(0).toString();
        m_logger->info("{} Downloading purchased track", m_loggingPrefix);
        QString fileExt = ".mp4";
        if (url.contains("mp3g"))
            fileExt = ".zip";
        downloadFile(url, QString(dlSongId + " - " + dlArtist + " - " + dlTitle + fileExt));
        m_logger->info("{} Download complete", m_loggingPrefix);

    } else if ((json.object().value("result").toString() == "ERROR") &&
               (json.object().value("error").toString() == "Payment failed. Check your credit card details.")) {
        emit paymentProcessingFailed();
    } else {
        m_logger->warn("{} Unexpected JSON data received: {}", m_loggingPrefix, data.toStdString());
        if (requestType == "login") {
            knLoginError = true;
            knSessionId.clear();
            emit knLoginFailure();
        } else if (requestType == "purchase") {
            emit paymentProcessingFailed();
        }
    }
}

void SongShop::onDownloadProgress(qint64 received, qint64 total) {
    m_logger->trace("{} Download progress: {} of {} downloaded", m_loggingPrefix, received, total);
    emit downloadProgress(received, total);
}
