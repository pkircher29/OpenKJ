#ifndef OPENKJ_REQUESTSEARCHURL_H
#define OPENKJ_REQUESTSEARCHURL_H

#include <QUrl>
#include <QUrlQuery>

inline QUrl requestSearchUrl(const QString &searchText)
{
    QUrl url(QStringLiteral("https://db.openkj.org/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("type"), QStringLiteral("All"));
    // QUrlQuery preserves percent escapes and literal '+'. Encode the input
    // first so a typed "%26" remains text and form-style servers preserve '+'.
    query.addQueryItem(QStringLiteral("searchstr"), QString::fromLatin1(QUrl::toPercentEncoding(searchText)));
    url.setQuery(query);
    return url;
}

#endif
