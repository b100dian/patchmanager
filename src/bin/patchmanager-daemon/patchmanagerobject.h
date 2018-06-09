/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#ifndef PATCHMANAGEROBJECT_H
#define PATCHMANAGEROBJECT_H

#include "journal.h"

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtCore/QVariantList>
#include <QtCore/QDir>

#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QFileSystemWatcher>

#include <systemd/sd-journal.h>

#ifndef SERVER_URL
#define SERVER_URL          "https://coderus.openrepos.net"
#endif

#ifndef API_PATH
#define API_PATH            "pm2/api"
#endif

#define PROJECTS_PATH       "projects"
#define PROJECT_PATH        "project"
#define RATING_PATH         "rating"
#define FILES_PATH          "files"
#define MEDIA_PATH          "media"

#define CATALOG_URL         SERVER_URL "/" API_PATH
#define MEDIA_URL           SERVER_URL "/" MEDIA_PATH

class QTimer;
class QSettings;
class QNetworkAccessManager;
class PatchManagerAdaptor;
class QLocalServer;
class PatchManagerObject : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit PatchManagerObject(QObject *parent = nullptr);
    virtual ~PatchManagerObject();

    enum NotifyAction {
        NotifyActionSuccessApply,
        NotifyActionSuccessUnapply,
        NotifyActionFailedApply,
        NotifyActionFailedUnapply,
        NotifyActionUpdateAvailable,
    };
    Q_ENUM(NotifyAction)

public slots:
    void process();

    QVariantList listPatches();
    QVariantMap listVersions();
    bool isPatchApplied(const QString &patch);
    bool applyPatch(const QString &patch);
    bool unapplyPatch(const QString &patch);
    bool unapplyAllPatches();
    bool installPatch(const QString &patch, const QString &version, const QString &url);
    bool uninstallPatch(const QString &patch);
    bool resetPatchState(const QString &patch);

    int checkVote(const QString &patch);
    void votePatch(const QString &patch, int action);

    QString checkEaster();

    QVariantList downloadCatalog(const QVariantMap &params);
    QVariantMap downloadPatchInfo(const QString &name);
    void checkForUpdates();

    QVariantMap getUpdates() const;

    bool putSettings(const QString & name, const QDBusVariant & value);
    QDBusVariant getSettings(const QString & name, const QDBusVariant & def);

    bool putSettings(const QString & name, const QVariant & value);
    QVariant getSettings(const QString & name, const QVariant & def);

    static QString maxVersion(const QString &version1, const QString &version2);

    void restartServices();
    void patchToggleService(const QString &patch, bool activate);

    bool getToggleServices() const;
    bool getFailure() const;
    void resolveFailure();

    QString getPatchmanagerVersion() const;
    QString getSsuVersion() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private slots:
    void onLipstickChanged(const QString &, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void onOsUpdateProgress(int progress);
    void onTimerAction();

    void startReadingLocalServer();

    void onOriginalFileChanged(const QString &path);

    void onFailureOccured();

    void doRegisterDBus();
    void doPrepareCacheRoot();
    void doPrepareCache(const QString &patchName, bool apply = true);
    void doStartLocalServer();

    void doRefreshPatchList();
    void doListPatches(const QDBusMessage &message);

    bool doPatch(const QString &patchName, bool apply);
    void doPatch(const QVariantMap &params, const QDBusMessage &message, bool apply);
    void doResetPatchState(const QString &patch, const QDBusMessage &message);

    void doInstallPatch(const QVariantMap &params, const QDBusMessage &message);
    void downloadPatchArchive(const QVariantMap &params, const QDBusMessage &message);

    void doUninstallPatch(const QString &patch, const QDBusMessage &message);

    void doCheckVote(const QString &patch, const QDBusMessage &message);
    void sendVote(const QString &patch, int action);

    void doCheckEaster(const QDBusMessage &message);

    void requestDownloadCatalog(const QVariantMap &params, const QDBusMessage &message);
    void requestDownloadPatchInfo(const QString &name, const QDBusMessage &message);
    void requestCheckForUpdates();

    void restartLipstick();

private:
    void resetSystem();

    void registerDBus();

    void startLocalServer();
    void initialize();

    QString checkRpmPatch(const QString &patch) const;

    int getVote(const QString &patch);

    void sendActivation(const QString & patch, const QString & version);

    void downloadPatch(const QString &patch, const QUrl &url, const QString &file);

    void sendMessageReply(const QDBusMessage &message, const QVariant &result);
    void sendMessageError(const QDBusMessage &message, const QString &errorString);

    QList<QVariantMap> listPatchesFromDir(const QString &dir, QSet<QString> &existingPatches, bool existing = true);
    bool makePatch(const QDir &root, const QString &patchPath, QVariantMap &patch, bool available);
    void notify(const QString &patch, PatchManagerObject::NotifyAction action);

    void getVersion();

    void lateInitialize();
    void refreshPatchList();
    void prepareCacheRoot();

    void eraseRecursively(const QString &path);

    bool checkIsFakeLinked(const QString &path);
    bool tryToLinkFakeParent(const QString &path);
    bool tryToUnlinkFakeParent(const QString &path);

    bool m_dbusRegistered = false;
    QSet<QString> m_appliedPatches;
    QMap<QString, QVariantMap> m_metadata;
    QTimer *m_timer;

    QMap<QString, QStringList> m_patchFiles;
    QMap<QString, QStringList> m_fileToPatch;
    QStringList m_patchedFiles;

    QVariantMap m_updates;

    QString m_ssuRelease;
    PatchManagerAdaptor *m_adaptor = nullptr;
    QNetworkAccessManager *m_nam;

    QFileSystemWatcher *m_originalWatcher;

    QSettings *m_settings;

    QThread *m_serverThread;
    QLocalServer *m_localServer;

    QHash<QString, QStringList> m_toggleServices; // category => patches

    Journal *m_journal;
    bool m_failed = false;
};

#endif // PATCHMANAGEROBJECT_H

