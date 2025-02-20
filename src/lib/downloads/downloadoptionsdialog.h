/* ============================================================
* Falkon - Qt web browser
* Copyright (C) 2010-2017 David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef DOWNLOADOPTIONSDIALOG_H
#define DOWNLOADOPTIONSDIALOG_H

#include <QDialog>
#include <QUrl>

#include "qzcommon.h"
#include "downloadmanager.h"
#include "downloaditem.h"

namespace Ui
{
class DownloadOptionsDialog;
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
class QWebEngineDownloadItem;
#else
class QWebEngineDownloadRequest;
#endif

class FALKON_EXPORT DownloadOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadOptionsDialog(const QString &fileName, Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS *downloadItem, QWidget* parent = nullptr);
    ~DownloadOptionsDialog() override;

    void showExternalManagerOption(bool show);
    void showFromLine(bool show);

    void setLastDownloadOption(const DownloadManager::DownloadOption &option);

    int exec() override;

private Q_SLOTS:
    void copyDownloadLink();
    void emitDialogFinished(int status);

Q_SIGNALS:
    void dialogFinished(int);

private:
    Ui::DownloadOptionsDialog* ui;

    Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS *m_downloadItem;
    bool m_signalEmited;
};

#endif // DOWNLOADOPTIONSDIALOG_H
