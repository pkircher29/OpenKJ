#ifndef OPENKJ_UPDATEVERSION_H
#define OPENKJ_UPDATEVERSION_H

#include <QRegularExpression>
#include <QVersionNumber>
#include <optional>

inline std::optional<QVersionNumber> parseUpdateVersion(const QString &text)
{
    const QString version = text.trimmed();
    static const QRegularExpression pattern(QStringLiteral("\\A[0-9]+\\.[0-9]+\\.[0-9]+\\z"));
    if (!pattern.match(version).hasMatch())
        return std::nullopt;
    const auto parts = version.split('.');
    bool majorOk, minorOk, patchOk;
    const int major = parts.at(0).toInt(&majorOk);
    const int minor = parts.at(1).toInt(&minorOk);
    const int patch = parts.at(2).toInt(&patchOk);
    if (!majorOk || !minorOk || !patchOk)
        return std::nullopt;
    return QVersionNumber(major, minor, patch);
}

#endif
