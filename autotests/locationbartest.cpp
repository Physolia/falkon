/* ============================================================
* Falkon - Qt web browser
* Copyright (C) 2018 David Rosca <nowrep@gmail.com>
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
#include "locationbartest.h"
#include "locationbar.h"
#include "mainapplication.h"
#include "searchenginesmanager.h"
#include "bookmarks.h"
#include "bookmarkitem.h"

#include <QtTest/QtTest>

static void removeBookmarks(BookmarkItem *parent)
{
    for (BookmarkItem *child : parent->children()) {
        mApp->bookmarks()->removeBookmark(child);
        removeBookmarks(child);
    }
}

void LocationBarTest::initTestCase()
{
}

void LocationBarTest::cleanupTestCase()
{
}

void LocationBarTest::init()
{
    mApp->searchEnginesManager()->setAllEngines({});
    removeBookmarks(mApp->bookmarks()->rootItem());
}

void LocationBarTest::loadActionBasicTest()
{
    LocationBar::LoadAction action;

    action = LocationBar::loadAction("http://kde.org");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://kde.org"));

    action = LocationBar::loadAction("kde.org");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://kde.org"));

    action = LocationBar::loadAction("localhost");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://localhost"));

    action = LocationBar::loadAction("localhost/test/path?x=2");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://localhost/test/path?x=2"));

    action = LocationBar::loadAction("host.com/test/path?x=2");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://host.com/test/path?x=2"));

    action = LocationBar::loadAction("not-url");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);

    action = LocationBar::loadAction("not url with spaces");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);

    action = LocationBar::loadAction("qupzilla:about");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("qupzilla:about"));
}

void LocationBarTest::loadActionBookmarksTest()
{
    BookmarkItem* bookmark = new BookmarkItem(BookmarkItem::Url);
    bookmark->setTitle("KDE Bookmark title");
    bookmark->setUrl(QUrl("http://kde.org"));
    bookmark->setKeyword("kde-bookmark");
    mApp->bookmarks()->addBookmark(mApp->bookmarks()->unsortedFolder(), bookmark);

    LocationBar::LoadAction action;

    action = LocationBar::loadAction("http://kde.org");
    QCOMPARE(action.type, LocationBar::LoadAction::Url);
    QCOMPARE(action.loadRequest.url(), QUrl("http://kde.org"));

    action = LocationBar::loadAction("kde-bookmark-notkeyword");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);

    action = LocationBar::loadAction("kde-bookmark");
    QCOMPARE(action.type, LocationBar::LoadAction::Bookmark);
    QCOMPARE(action.bookmark, bookmark);
    QCOMPARE(action.loadRequest.url(), QUrl("http://kde.org"));
}

void LocationBarTest::loadActionSearchTest()
{
    SearchEngine engine;
    engine.name = "Test Engine";
    engine.url = "http://test/%s";
    engine.shortcut = "t";
    mApp->searchEnginesManager()->addEngine(engine);
    mApp->searchEnginesManager()->setActiveEngine(engine);

    LocationBar::LoadAction action;

    action = LocationBar::loadAction("search term");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);
    QCOMPARE(action.loadRequest.url(), QUrl("http://test/search%20term"));

    action = LocationBar::loadAction("t search term");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);
    QCOMPARE(action.loadRequest.url(), QUrl("http://test/search%20term"));

    action = LocationBar::loadAction(" ttt-notsearch");
    QCOMPARE(action.type, LocationBar::LoadAction::Search);
    QCOMPARE(action.loadRequest.url(), QUrl("http://test/ttt-notsearch"));
}

int main(int argc, char **argv)
{
    MainApplication::setTestModeEnabled(true);
    MainApplication app(argc, argv);

    LocationBarTest test;
    return QTest::qExec(&test, argc, argv);
}
