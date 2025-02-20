/* ============================================================
* Falkon - Qt web browser
* Copyright (C) 2010-2018 David Rosca <nowrep@gmail.com>
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
#include "downloaditem.h"
#include "ui_downloaditem.h"
#include "mainapplication.h"
#include "browserwindow.h"
#include "tabwidget.h"
#include "webpage.h"
#include "downloadmanager.h"
#include "networkmanager.h"
#include "qztools.h"
#include "datapaths.h"

#include <QMenu>
#include <QClipboard>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QTimer>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileIconProvider>
#include <QDesktopServices>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QWebEngineDownloadItem>
#else
#include <QWebEngineDownloadRequest>
#endif
#include <QtWebEngineWidgetsVersion>

#ifdef Q_OS_WIN
#include "Shlwapi.h"
#include "shellapi.h"
#endif

//#define DOWNMANAGER_DEBUG

DownloadItem::DownloadItem(QListWidgetItem *item, Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS* downloadItem, const QString &path, const QString &fileName, bool openFile, DownloadManager* manager)
    : QWidget()
    , ui(new Ui::DownloadItem)
    , m_item(item)
    , m_download(downloadItem)
    , m_path(path)
    , m_fileName(fileName)
    , m_downUrl(downloadItem->url())
    , m_openFile(openFile)
    , m_downloading(false)
    , m_downloadStopped(false)
    , m_currSpeed(0)
    , m_received(downloadItem->receivedBytes())
    , m_total(downloadItem->totalBytes())
{
#ifdef DOWNMANAGER_DEBUG
    qDebug() << __FUNCTION__ << item << reply << path << fileName;
#endif

    ui->setupUi(this);
    setMaximumWidth(525);

    ui->cancelButton->setPixmap(QIcon::fromTheme(QSL("process-stop")).pixmap(20, 20));
    ui->pauseResumeButton->setPixmap(QIcon::fromTheme(QSL("media-playback-pause")).pixmap(20, 20));
    ui->fileName->setText(m_fileName);
    ui->downloadInfo->setText(tr("Remaining time unavailable"));

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
    connect(ui->cancelButton, &ClickableLabel::clicked, this, &DownloadItem::stop);
    connect(ui->pauseResumeButton, &ClickableLabel::clicked, this, &DownloadItem::pauseResume);
    connect(manager, &DownloadManager::resized, this, &DownloadItem::parentResized);
}

void DownloadItem::startDownloading()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    connect(m_download, &QWebEngineDownloadItem::finished, this, &DownloadItem::finished);
    connect(m_download, &QWebEngineDownloadItem::downloadProgress, this, &DownloadItem::downloadProgress);
#else
    connect(m_download, &QWebEngineDownloadRequest::isFinishedChanged, this, &DownloadItem::finished);
    connect(m_download, &QWebEngineDownloadRequest::receivedBytesChanged, this, &DownloadItem::receivedOrTotalBytesChanged);
    connect(m_download, &QWebEngineDownloadRequest::totalBytesChanged, this, &DownloadItem::receivedOrTotalBytesChanged);
#endif

    m_downloading = true;
    if (!m_downTimer.isValid()) {
        m_downTimer.start();
    }

    updateDownloadInfo(0, m_download->receivedBytes(), m_download->totalBytes());

#ifdef Q_OS_LINUX
    // QFileIconProvider uses only suffix on Linux
    QFileIconProvider iconProvider;
    QIcon fileIcon = iconProvider.icon(QFileInfo(m_fileName));
    if (!fileIcon.isNull()) {
        ui->fileIcon->setPixmap(fileIcon.pixmap(30));
    } else {
        ui->fileIcon->setPixmap(style()->standardIcon(QStyle::SP_FileIcon).pixmap(30));
    }
#else
    ui->fileIcon->hide();
#endif
}

void DownloadItem::parentResized(const QSize &size)
{
    if (size.width() < 200) {
        return;
    }
    setMaximumWidth(size.width());
}

void DownloadItem::finished()
{
#ifdef DOWNMANAGER_DEBUG
    qDebug() << __FUNCTION__ << m_reply;
#endif

    bool success = false;
    QString host = m_download->url().host();

    switch (m_download->state()) {
    case Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS::DownloadCompleted:
        success = true;
        ui->downloadInfo->setText(tr("Done - %1 (%2)").arg(host, QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
        break;

    case Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS::DownloadInterrupted:
        ui->downloadInfo->setText(tr("Error - %1").arg(host));
        break;

    case Q_WEB_ENGINE_DOWNLOAD_ITEM_CLASS::DownloadCancelled:
        ui->downloadInfo->setText(tr("Cancelled - %1").arg(host));
        break;

    default:
        break;
    }

    ui->progressBar->hide();
    ui->cancelButton->hide();
    ui->pauseResumeButton->hide();
    ui->frame->hide();

    m_item->setSizeHint(sizeHint());
    m_downloading = false;

    if (success && m_openFile)
        openFile();

    Q_EMIT downloadFinished(true);
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
void DownloadItem::downloadProgress(qint64 received, qint64 total)
{
#else
void DownloadItem::receivedOrTotalBytesChanged()
{
    qint64 received = m_download->receivedBytes();
    qint64 total = m_download->totalBytes();
#endif
#ifdef DOWNMANAGER_DEBUG
    qDebug() << __FUNCTION__ << received << total;
#endif
    qint64 currentValue = 0;
    qint64 totalValue = 0;
    if (total > 0) {
        currentValue = received * 100 / total;
        totalValue = 100;
    }
    ui->progressBar->setValue(currentValue);
    ui->progressBar->setMaximum(totalValue);
    m_currSpeed = received * 1000.0 / m_downTimer.elapsed();
    m_received = received;
    m_total = total;

    updateDownloadInfo(m_currSpeed, m_received, m_total);
    emit progressChanged(m_currSpeed, m_received, m_total);
}

int DownloadItem::progress()
{
    return ui->progressBar->value();
}

bool DownloadItem::isCancelled()
{
    return ui->downloadInfo->text().startsWith(tr("Cancelled"));
}

QString DownloadItem::remaingTimeToString(QTime time)
{
    if (time < QTime(0, 0, 10)) {
        return tr("few seconds");
    }
    else if (time < QTime(0, 1)) {
        //~ singular %n second
        //~ plural %n seconds
        return tr("%n seconds", "", time.second());
    }
    else if (time < QTime(1, 0)) {
        //~ singular %n minute
        //~ plural %n minutes
        return tr("%n minutes", "", time.minute());
    }
    else {
        //~ singular %n hour
        //~ plural %n hours
        return tr("%n hours", "", time.hour());
    }
}

QString DownloadItem::currentSpeedToString(double speed)
{
    if (speed < 0) {
        return tr("Unknown speed");
    }

    speed /= 1024; // kB
    if (speed < 1000) {
        return QString::number(speed, 'f', 0) + QLatin1String(" ") + tr("kB/s");
    }

    speed /= 1024; //MB
    if (speed < 1000) {
        return QString::number(speed, 'f', 2) + QLatin1String(" ") + tr("MB/s");
    }

    speed /= 1024; //GB
    return QString::number(speed, 'f', 2) + QLatin1String(" ") + tr("GB/s");
}

void DownloadItem::updateDownloadInfo(double currSpeed, qint64 received, qint64 total)
{
#ifdef DOWNMANAGER_DEBUG
    qDebug() << __FUNCTION__ << currSpeed << received << total;
#endif
    //            QString          QString       QString     QString
    //          | m_remTime |   |m_currSize|  |m_fileSize|  |m_speed|
    // Remaining 26 minutes -     339MB of      693 MB        (350kB/s)

    if (m_download->isPaused()) {
        return;
    }

    int estimatedTime = ((total - received) / 1024) / (currSpeed / 1024);
    QString speed = currentSpeedToString(currSpeed);
    // We have QString speed now

    QTime time(0, 0, 0);
    time = time.addSecs(estimatedTime);
    QString remTime = remaingTimeToString(time);
    m_remTime = time;

    QString currSize = QzTools::fileSizeToString(received);
    QString fileSize = QzTools::fileSizeToString(total);

    if (fileSize == tr("Unknown size")) {
        ui->downloadInfo->setText(tr("%2 - unknown size (%3)").arg(currSize, speed));
    }
    else {
        ui->downloadInfo->setText(tr("Remaining %1 - %2 of %3 (%4)").arg(remTime, currSize, fileSize, speed));
    }
}

void DownloadItem::stop()
{
#ifdef DOWNMANAGER_DEBUG
    qDebug() << __FUNCTION__;
#endif
    if (m_downloadStopped) {
        return;
    }
    m_downloadStopped = true;
    ui->progressBar->hide();
    ui->cancelButton->hide();
    ui->pauseResumeButton->hide();
    m_item->setSizeHint(sizeHint());
    ui->downloadInfo->setText(tr("Cancelled - %1").arg(m_download->url().host()));
    m_download->cancel();
    m_downloading = false;

    Q_EMIT downloadFinished(false);
}

void DownloadItem::pauseResume()
{
    if (m_download->isPaused()) {
        m_download->resume();
        ui->pauseResumeButton->setPixmap(QIcon::fromTheme(QSL("media-playback-pause")).pixmap(20, 20));
    } else {
        m_download->pause();
        ui->pauseResumeButton->setPixmap(QIcon::fromTheme(QSL("media-playback-start")).pixmap(20, 20));
        ui->downloadInfo->setText(tr("Paused - %1").arg(m_download->url().host()));
    }
}

void DownloadItem::mouseDoubleClickEvent(QMouseEvent* e)
{
    openFile();
    e->accept();
}

void DownloadItem::customContextMenuRequested(const QPoint &pos)
{
    QMenu menu;
    menu.addAction(QIcon::fromTheme("document-open"), tr("Open File"), this, &DownloadItem::openFile);

    menu.addAction(tr("Open Folder"), this, &DownloadItem::openFolder);
    menu.addSeparator();
    menu.addAction(QIcon::fromTheme("edit-copy"), tr("Copy Download Link"), this, &DownloadItem::copyDownloadLink);
    menu.addSeparator();
    menu.addAction(QIcon::fromTheme("process-stop"), tr("Cancel downloading"), this, &DownloadItem::stop)->setEnabled(m_downloading);

    if (m_download->isPaused()) {
        menu.addAction(QIcon::fromTheme("media-playback-start"), tr("Resume downloading"), this, &DownloadItem::pauseResume)->setEnabled(m_downloading);
    } else {
        menu.addAction(QIcon::fromTheme("media-playback-pause"), tr("Pause downloading"), this, &DownloadItem::pauseResume)->setEnabled(m_downloading);
    }

    menu.addAction(QIcon::fromTheme("list-remove"), tr("Remove From List"), this, &DownloadItem::clear)->setEnabled(!m_downloading);

    if (m_downloading || ui->downloadInfo->text().startsWith(tr("Cancelled")) || ui->downloadInfo->text().startsWith(tr("Error"))) {
        menu.actions().at(0)->setEnabled(false);
    }
    menu.exec(mapToGlobal(pos));
}

void DownloadItem::copyDownloadLink()
{
    QApplication::clipboard()->setText(m_downUrl.toString());
}

void DownloadItem::clear()
{
    Q_EMIT deleteItem(this);
}

void DownloadItem::openFile()
{
    if (m_downloading) {
        return;
    }
    QFileInfo info(m_path, m_fileName);
    if (info.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
    }
    else {
        QMessageBox::warning(m_item->listWidget()->parentWidget(), tr("Not found"), tr("Sorry, the file \n %1 \n was not found!").arg(info.absoluteFilePath()));
    }
}

void DownloadItem::openFolder()
{
#ifdef Q_OS_WIN
    QString winFileName = QSL("%1/%2").arg(m_path, m_fileName);

    if (m_downloading) {
        winFileName.append(QSL(".download"));
    }

    winFileName.replace(QLatin1Char('/'), "\\");
    QString shExArg = "/e,/select,\"" + winFileName + "\"";
    ShellExecute(NULL, NULL, TEXT("explorer.exe"), shExArg.toStdWString().c_str(), NULL, SW_SHOW);
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_path));
#endif
}

QUrl DownloadItem::url() const
{
    return m_downUrl;
}

QString DownloadItem::path() const
{
    return m_path;
}

QString DownloadItem::fileName() const
{
    return m_fileName;
}

DownloadItem::~DownloadItem()
{
    delete ui;
    delete m_item;
}
