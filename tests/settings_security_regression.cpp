#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <iostream>

namespace {

bool require(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}

#ifndef Q_OS_LINUX
QString settingsFilePath()
{
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dataDir.mkpath(dataDir.absolutePath());
    return dataDir.absoluteFilePath("openkj.ini");
}
#endif

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");

#ifdef Q_OS_LINUX
    {
        QSettings legacy;
        legacy.setValue("cc", "fake-legacy-obfuscated-card-data");
        legacy.setValue("saveCC", true);
        legacy.sync();
    }
#else
    {
        QSettings legacy(settingsFilePath(), QSettings::IniFormat);
        legacy.setValue("cc", "fake-legacy-obfuscated-card-data");
        legacy.setValue("saveCC", true);
        legacy.sync();
    }
#endif

    Settings settings;

    bool ok = true;
#ifdef Q_OS_LINUX
    QSettings verification;
#else
    QSettings verification(settingsFilePath(), QSettings::IniFormat);
#endif
    verification.sync();
    ok &= require(!verification.contains("cc"), "legacy card data was not purged");
    ok &= require(!verification.contains("saveCC"), "legacy save-card preference was not purged");

    settings.setCC("4111111111111111", "01", "2030", "123", "password");
    settings.setSaveCC(true);
    verification.sync();
    ok &= require(!settings.saveCC(), "card persistence was re-enabled");
    ok &= require(settings.getCCN("password").isEmpty(), "card number was returned from settings");
    ok &= require(settings.getCCV("password").isEmpty(), "CVV was returned from settings");
    ok &= require(!verification.contains("cc"), "setCC persisted payment-card data");
    ok &= require(!verification.contains("saveCC"), "setSaveCC persisted the preference");

    if (ok)
        std::cout << "PASS: legacy payment-card settings are purged and cannot be persisted\n";
    return ok ? 0 : 1;
}
