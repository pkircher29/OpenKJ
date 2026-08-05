#include "okjsongbookapi.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include "idledetect.h"

extern IdleDetect *filter;

std::ostream &operator<<(std::ostream &os, const OkjsVenue &v) {
    return os << "venue_id: " << v.venueId
              << "name: " << v.name
              << "urlName: " << v.urlName
              << "accepting: " << v.accepting;
}

OKJSongbookAPI::OKJSongbookAPI(QObject *parent) : QObject(parent)
{
    m_logger = spdlog::get("logger");
    programIsIdle = false;
    delayErrorEmitted = false;
    connectionReset = false;
    cancelUpdate = false;
    updateInProgress = false;
    serial = 0;
    entitledSystems = 1;
    timer = new QTimer(this);
    timer->setInterval(m_settings.requestServerInterval() * 1000);
    alertTimer = new QTimer(this);
    alertTimer->start(600000);
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::sslErrors, this, &OKJSongbookAPI::onSslErrors);
    connect(manager, &QNetworkAccessManager::finished, this, &OKJSongbookAPI::onNetworkReply);
    connect(timer, &QTimer::timeout, this, &OKJSongbookAPI::timerTimeout);
    connect(alertTimer, &QTimer::timeout, this, &OKJSongbookAPI::alertTimerTimeout);
    connect(filter, &IdleDetect::idleStateChanged, this, &OKJSongbookAPI::idleStateChanged);
    if (m_settings.requestServerEnabled())
    {
        getEntitledSystemCount();
        refreshVenues();
        alertCheck();
    }
    timer->start();
}

void OKJSongbookAPI::getSerial()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","getSerial");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::refreshRequests()
{
    QJsonObject jsonObject;
    jsonObject.insert("api_key", m_settings.requestServerApiKey());
    jsonObject.insert("command","getRequests");
    jsonObject.insert("venue_id", m_settings.requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::triggerTestAdd()
{
    QJsonObject jsonObject;
    jsonObject.insert("command","testingAddRandomRequest");
    jsonObject.insert("venue_id", m_settings.requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}


void OKJSongbookAPI::removeRequest(int requestId)
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","deleteRequest");
    mainObject.insert("venue_id", m_settings.requestServerVenue());
    mainObject.insert("request_id", requestId);
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

bool OKJSongbookAPI::getAccepting()
{
    for (const auto & venue : venues)
    {
        if (venue.venueId == m_settings.requestServerVenue())
            return venue.accepting;
    }
    return false;
}

void OKJSongbookAPI::setAccepting(bool enabled)
{
    alertCheck();
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","setAccepting");
    mainObject.insert("venue_id", m_settings.requestServerVenue());
    mainObject.insert("accepting", enabled);
    mainObject.insert("system_id", m_settings.systemId());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::refreshVenues(bool blocking)
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","getVenues");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    if (blocking)
    {
        while (!reply->isFinished())
            QApplication::processEvents();
    }
}

void OKJSongbookAPI::clearRequests()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","clearRequests");
    mainObject.insert("venue_id", m_settings.requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::updateSongDb()
{
    cancelUpdate = false;
    updateFailed = false;
    updateInProgress = true;
    emit remoteSongDbUpdateStart();
    const int songsPerDoc = 1000;
    QList<QJsonDocument> jsonDocs;
    QSqlQuery query;
    auto finishUpdate = [this]() {
        updateInProgress = false;
        emit remoteSongDbUpdateDone();
    };
    auto makeDocument = [this](const QJsonArray &songs) {
        QJsonObject object;
        object.insert("api_key", m_settings.requestServerApiKey());
        object.insert("command", "addSongs");
        object.insert("songs", songs);
        object.insert("system_id", m_settings.systemId());
        return QJsonDocument(object);
    };

    if (!query.exec("SELECT DISTINCT artist,title FROM dbsongs WHERE discid != '!!DROPPED!!' AND discid != '!!BAD!!' ORDER BY artist ASC, title ASC")) {
        m_logger->error("{} Failed to enumerate songs for remote update: {}", m_loggingPrefix,
                        query.lastError().text());
        updateFailed = true;
        finishUpdate();
        return;
    }

    QJsonArray songsArray;
    while (query.next()) {
        if (cancelUpdate) {
            finishUpdate();
            return;
        }
        QJsonObject songObject;
        songObject.insert("artist", query.value(0).toString());
        songObject.insert("title", query.value(1).toString());
        songsArray.append(songObject);
        if (songsArray.size() == songsPerDoc) {
            jsonDocs.append(makeDocument(songsArray));
            songsArray = QJsonArray();
        }
        QApplication::processEvents();
    }
    if (!songsArray.isEmpty())
        jsonDocs.append(makeDocument(songsArray));

    emit remoteSongDbUpdateNumDocs(jsonDocs.size());

    QNetworkAccessManager networkManager;
    const QUrl url(m_settings.requestServerUrl());
    auto postDocument = [&](const QJsonDocument &document) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        request.setTransferTimeout(30000);
#endif
        QNetworkReply *reply = networkManager.post(request, document.toJson());
        while (!reply->isFinished()) {
            QApplication::processEvents();
            if (cancelUpdate)
                reply->abort();
        }
        const QByteArray response = reply->readAll();
        const bool success = reply->error() == QNetworkReply::NoError && !cancelUpdate;
        if (!success && !cancelUpdate)
            m_logger->error("{} Remote song database update failed: {}", m_loggingPrefix,
                            reply->errorString());
        else
            m_logger->trace("{} Got reply: {}", m_loggingPrefix, response.toStdString());
        delete reply;
        return success;
    };

    QJsonObject clearObject;
    clearObject.insert("api_key", m_settings.requestServerApiKey());
    clearObject.insert("command", "clearDatabase");
    clearObject.insert("system_id", m_settings.systemId());
    if (!postDocument(QJsonDocument(clearObject))) {
        updateFailed = !cancelUpdate;
        finishUpdate();
        return;
    }

    for (int i = 0; i < jsonDocs.size(); ++i) {
        if (!postDocument(jsonDocs.at(i))) {
            updateFailed = !cancelUpdate;
            finishUpdate();
            return;
        }
        emit remoteSongDbUpdateProgress(i + 1);
    }
    finishUpdate();
}

bool OKJSongbookAPI::test()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","getSerial");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkAccessManager m_NetworkMngr;

    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_NetworkMngr.post(request, jsonDocument.toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError)
    {
        m_logger->error("{} Network error: {}", m_loggingPrefix, reply->errorString());
        emit testFailed(reply->errorString());
        delete reply;
        return false;
    }
    QByteArray data = reply->readAll();
    delete reply;
    QJsonDocument json = QJsonDocument::fromJson(data);
    m_logger->trace("{} Got server response: {}", m_loggingPrefix, json.toJson().toStdString());
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    if (json.object().value("errorString").toString() != "")
    {
        m_logger->warn("{} Got error reply: {}", m_loggingPrefix, json.object().value("errorString").toString());
        emit testFailed(json.object().value("errorString").toString());
        return false;
    }
    if (command == "getSerial")
    {
        int newSerial = json.object().value("serial").toInt();
        if (newSerial != 0)
        {
            emit testPassed();
            return true;
        }
    }
    emit testFailed("Unknown error");
    return false;
}

void OKJSongbookAPI::alertCheck()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","getAlert");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(m_settings.requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::onSslErrors(QNetworkReply *reply, const QList<QSslError>& errors)
{
    Q_UNUSED(errors)
    static QString lastUrl;
    static bool errorEmitted = false;
    if (lastUrl != m_settings.requestServerUrl())
        errorEmitted = false;
    if (m_settings.requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    else if (!errorEmitted)
    {
        emit sslError();
        errorEmitted = true;

    }
    lastUrl = m_settings.requestServerUrl();
}

void OKJSongbookAPI::onTestSslErrors(QNetworkReply *reply, QList<QSslError> errors)
{
    if (m_settings.requestServerIgnoreCertErrors())
    {
        reply->ignoreSslErrors();
        return;
    }
    QString errorText;
    foreach (QSslError error, errors) {
        errorText += error.errorString() + "\n";
    }
    emit testSslError(errorText);
}

void OKJSongbookAPI::onNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (m_settings.requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    if (reply->error() != QNetworkReply::NoError)
    {
        m_logger->warn("{} Network error: {}", m_loggingPrefix, reply->errorString());
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    if (error)
    {
        m_logger->warn("{} Got error reply: {}", m_loggingPrefix, json.object().value("errorString").toString());
        return;
    }
    if (command == "testingAddRandomRequest")
    {
        refreshRequests();
    }
    if (command == "getEntitledSystemCount")
    {
        entitledSystems = json.object().value("count").toInt();
        emit entitledSystemCountChanged(entitledSystems);
        m_logger->info("{} Server reports entitlements for {} concurrent systems", m_loggingPrefix, entitledSystems);
    }
    if (command == "getAlert")
    {
        if(json.object().value("alert").toBool())
        {
            emit alertRecieved(json.object().value("title").toString(), json.object().value("message").toString());
            venues.clear();
            refreshVenues();
        }
    }
    if (command == "getSerial")
    {
        int newSerial = json.object().value("serial").toInt();
        if (newSerial == 0)
        {
            m_logger->warn("{} Server didn't returen a valid serial!", m_loggingPrefix);
            return;
        }
        if (serial == newSerial)
        {
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
        }
        else
        {
            serial = newSerial;
            refreshRequests();
            refreshVenues();
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
        }
    }
    if (command == "getVenues")
    {
        QJsonArray venuesArray = json.object().value("venues").toArray();
        OkjsVenues l_venues;
        for (const auto &venuesEntry : venuesArray)
        {
            OkjsVenue venue;
            QJsonObject jsonObject = venuesEntry.toObject();
            venue.venueId = jsonObject.value("venue_id").toInt();
            venue.name = jsonObject.value("name").toString();
            venue.urlName = jsonObject.value("url_name").toString();
            venue.accepting = jsonObject.value("accepting").toBool();
            l_venues.append(venue);
        }
        if (venues != l_venues)
        {
            venues = l_venues;
            emit venuesChanged(venues);
            getEntitledSystemCount();
        }
        lastSync = QTime::currentTime();
        emit synchronized(lastSync);
    }
    if (command == "clearRequests")
    {
        refreshRequests();
        refreshVenues();
    }
    if (command == "getRequests")
    {
        QJsonArray requestsArray = json.object().value("requests").toArray();
        OkjsRequests l_requests;
        for (const auto &requestEntry : requestsArray)
        {
            OkjsRequest request;
            QJsonObject jsonObject = requestEntry.toObject();
            request.requestId = jsonObject.value("request_id").toInt();
            request.artist = jsonObject.value("artist").toString();
            request.title = jsonObject.value("title").toString();
            request.singer = jsonObject.value("singer").toString();
            request.time = jsonObject.value("request_time").toInt();
            request.key = jsonObject.value("key_change").toInt();
            l_requests.append(request);
        }
        if (requests != l_requests)
        {
            requests = l_requests;
            emit requestsChanged(requests);
        }
        lastSync = QTime::currentTime();
        emit synchronized(lastSync);
    }
    if (command == "setAccepting")
    {
        refreshVenues();
        refreshRequests();
    }
    if (command == "deleteRequest")
    {
        refreshRequests();
        refreshVenues();
    }
}

void OKJSongbookAPI::timerTimeout()
{
    if (m_settings.requestServerEnabled() && !programIsIdle)
    {
        if ((lastSync.secsTo(QTime::currentTime()) > 300) && (!delayErrorEmitted))
        {
            emit delayError(lastSync.secsTo(QTime::currentTime()));
            delayErrorEmitted = true;
        }
        else if ((lastSync.secsTo(QTime::currentTime()) > 200) && (!connectionReset))
        {
            refreshRequests();
            refreshVenues();
            connectionReset = true;
        }
        else
        {
            connectionReset = false;
            delayErrorEmitted = false;
        }
        getSerial();
    }
}

void OKJSongbookAPI::alertTimerTimeout()
{
    if (m_settings.requestServerEnabled())
        alertCheck();
}

void OKJSongbookAPI::setInterval(int interval)
{
    timer->setInterval(interval * 1000);
}

void OKJSongbookAPI::idleStateChanged(bool isIdle)
{
    if (!isIdle)
    {
        // reset last update time to current to avoid showing
        // warning dialog to user due to updates being suppressed
        // during idle period
        lastSync = QTime::currentTime();
        timerTimeout();
    }
    programIsIdle = isIdle;
    m_logger->info("{} Program idle state changed to: {}", m_loggingPrefix, isIdle);
}

void OKJSongbookAPI::getEntitledSystemCount()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", m_settings.requestServerApiKey());
    mainObject.insert("command","getEntitledSystemCount");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(m_settings.requestServerUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::dbUpdateCanceled()
{
    m_logger->info("{} Remote db update cancelled by user", m_loggingPrefix);
    if (!cancelUpdate && updateInProgress)
    {
        QMessageBox msgBox(nullptr);
        msgBox.setWindowTitle(tr("Cancelling Update"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Are you sure you want to cancel the Songbook DB update?\n\nYour previous Songbook DB contents have already been cleared.\n\nCancelling now will result in an incomplete database of songs on your Songbook account.\n");
   //     msgBox.setInformativeText("Are you sure?  Your previous Songbook DB contents have already been cleared.\nCancelling now will result in an incomplete database of songs on your Songbook account.");
        QPushButton *yesButton = msgBox.addButton(tr("Cancel Update"), QMessageBox::AcceptRole);
        msgBox.addButton(tr("Continue Update"), QMessageBox::RejectRole);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton)
        {
            cancelUpdate = true;
            updateInProgress = false;
        }
    }
}

bool OkjsVenue::operator ==(const OkjsVenue &v) const
{
    if (venueId != v.venueId)
        return false;
    if (name != v.name)
        return false;
    if (urlName != v.urlName)
        return false;
    if (accepting != v.accepting)
        return false;
    return true;
}

bool OkjsRequest::operator ==(const OkjsRequest& r) const
{
    if (r.requestId != requestId)
        return false;
    if (r.artist != artist)
        return false;
    if (r.title != title)
        return false;
    if (r.time != time)
        return false;
    if (r.key != key)
        return false;
    if (r.singer != singer)
        return false;
    return true;
}

