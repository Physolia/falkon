/* ============================================================
* StatusBarIcons - Extra icons in statusbar for Falkon
* Copyright (C) 2013-2017 David Rosca <nowrep@gmail.com>
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
#include "sbi_networkicon.h"
#include "sbi_networkicondialog.h"
#include "sbi_networkproxy.h"
#include "sbi_networkmanager.h"
#include "mainapplication.h"
#include "browserwindow.h"

#include <QMenu>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QNetworkConfigurationManager>
#endif

SBI_NetworkIcon::SBI_NetworkIcon(BrowserWindow* window)
    : SBI_Icon(window)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    , m_networkConfiguration(new QNetworkConfigurationManager(this))
#endif
{
    setObjectName(QSL("sbi_networkicon"));
    setCursor(Qt::PointingHandCursor);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    onlineStateChanged(m_networkConfiguration->isOnline());

    connect(m_networkConfiguration, &QNetworkConfigurationManager::onlineStateChanged, this, &SBI_NetworkIcon::onlineStateChanged);
#else
    onlineStateChanged(true);
#endif
    connect(this, &ClickableLabel::clicked, this, &SBI_NetworkIcon::showMenu);
}

void SBI_NetworkIcon::onlineStateChanged(bool online)
{
    if (online) {
        setPixmap(QIcon(":sbi/data/network-online.png").pixmap(16));
    }
    else {
        setPixmap(QIcon(":sbi/data/network-offline.png").pixmap(16));
    }

    updateToolTip();
}

void SBI_NetworkIcon::showDialog()
{
    auto* dialog = new SBI_NetworkIconDialog(m_window);
    dialog->open();
}

void SBI_NetworkIcon::showMenu(const QPoint &pos)
{
    QFont boldFont = font();
    boldFont.setBold(true);

    QMenu menu;
    menu.addAction(QIcon::fromTheme("preferences-system-network", QIcon(":sbi/data/preferences-network.png")), tr("Proxy Configuration"))->setFont(boldFont);

    QMenu* proxyMenu = menu.addMenu(tr("Select proxy"));

    const QHash<QString, SBI_NetworkProxy*> &proxies = SBINetManager->proxies();

    QHashIterator<QString, SBI_NetworkProxy*> it(proxies);
    while (it.hasNext()) {
        it.next();
        QAction* act = proxyMenu->addAction(it.key(), this, &SBI_NetworkIcon::useProxy);
        act->setData(it.key());
        act->setCheckable(true);
        act->setChecked(it.value() == SBINetManager->currentProxy());
    }

    if (proxyMenu->actions().isEmpty()) {
        proxyMenu->addAction(tr("Empty"))->setEnabled(false);
    }

    menu.addSeparator();
    menu.addAction(tr("Manage proxies"), this, &SBI_NetworkIcon::showDialog);
    menu.exec(pos);
}

void SBI_NetworkIcon::useProxy()
{
    if (auto* act = qobject_cast<QAction*>(sender())) {
        SBINetManager->setCurrentProxy(act->data().toString());
    }
}

void SBI_NetworkIcon::updateToolTip()
{
    QString tooltip = tr("Shows network status and manages proxy<br/><br/><b>Network:</b><br/>%1<br/><br/><b>Proxy:</b><br/>%2");

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (m_networkConfiguration->isOnline()) {
        tooltip = tooltip.arg(tr("Connected"));
    }
    else {
        tooltip = tooltip.arg(tr("Offline"));
    }
#else
    // TODO QT6 - in Qt6 we're always reporting as online, should we just remove this functionality instead?
    // Or is there a way to detect network status in Qt6?
    tooltip = tooltip.arg(tr("Connected"));
#endif

    switch (QNetworkProxy::applicationProxy().type()) {
    case QNetworkProxy::DefaultProxy:
        tooltip = tooltip.arg(tr("System proxy"));
        break;

    case QNetworkProxy::NoProxy:
        tooltip = tooltip.arg(tr("No proxy"));
        break;

    default:
        tooltip = tooltip.arg(tr("User defined"));
        break;
    }

    if (SBINetManager->currentProxy()) {
        tooltip.append(QString(" (%1)").arg(SBINetManager->currentProxyName()));
    }

    setToolTip(tooltip);
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
void SBI_NetworkIcon::enterEvent(QEvent* event)
#else
void SBI_NetworkIcon::enterEvent(QEnterEvent* event)
#endif
{
    updateToolTip();

    SBI_Icon::enterEvent(event);
}
