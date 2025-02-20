/* ============================================================
* Falkon - Qt web browser
* Copyright (C) 2015-2018 David Rosca <nowrep@gmail.com>
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

#include "scripts.h"
#include "qztools.h"
#include "webpage.h"

#include <QUrlQuery>
#include <QtWebEngineWidgetsVersion>

QString Scripts::setupWebChannel()
{
    QString source =  QL1S("(function() {"
                           "%1"
                           ""
                           "function registerExternal(e) {"
                           "    window.external = e;"
                           "    if (window.external) {"
                           "        var event = document.createEvent('Event');"
                           "        event.initEvent('_falkon_external_created', true, true);"
                           "        window._falkon_external = true;"
                           "        document.dispatchEvent(event);"
                           "    }"
                           "}"
                           ""
                           "if (self !== top) {"
                           "    if (top._falkon_external)"
                           "        registerExternal(top.external);"
                           "    else"
                           "        top.document.addEventListener('_falkon_external_created', function() {"
                           "            registerExternal(top.external);"
                           "        });"
                           "    return;"
                           "}"
                           ""
                           "function registerWebChannel() {"
                           "    try {"
                           "        new QWebChannel(qt.webChannelTransport, function(channel) {"
                           "            var external = channel.objects.qz_object;"
                           "            external.extra = {};"
                           "            for (var key in channel.objects) {"
                           "                if (key != 'qz_object' && key.startsWith('qz_')) {"
                           "                    external.extra[key.substr(3)] = channel.objects[key];"
                           "                }"
                           "            }"
                           "            registerExternal(external);"
                           "        });"
                           "    } catch (e) {"
                           "        setTimeout(registerWebChannel, 100);"
                           "    }"
                           "}"
                           "registerWebChannel();"
                           ""
                           "})()");

    return source.arg(QzTools::readAllFileContents(QSL(":/qtwebchannel/qwebchannel.js")));
}

QString Scripts::setupFormObserver()
{
    QString source = QL1S("(function() {"
                          "    let eFormsOld  = [],"
                          "        eFormsDone = [];"
                          ""
                          "    function findUsername(inputs) {"
                          "        let usernameNames = ['user', 'name', 'login'];"
                          ""
                          "        for (let i = 0; i < usernameNames.length; ++i) {"
                          "            for (let j = 0; j < inputs.length; ++j)"
                          "                if (inputs[j].type == 'text' && inputs[j].value.length && inputs[j].name.indexOf(usernameNames[i]) != -1)"
                          "                    return inputs[j].value;"
                          "        }"
                          ""
                          "        for (let i = 0; i < inputs.length; ++i)"
                          "            if (inputs[i].type == 'text' && inputs[i].value.length)"
                          "                return inputs[i].value;"
                          ""
                          "        for (let i = 0; i < inputs.length; ++i)"
                          "            if (inputs[i].type == 'email' && inputs[i].value.length)"
                          "                return inputs[i].value;"
                          ""
                          "        return '';"
                          "    }"
                          ""
                          "    function processForm(eForm) {"
                          "        let data = '';"
                          "        let password = '';"
                          "        let inputs = eForm.getElementsByTagName('input');"
                          ""
                          "        for (let i = 0; i < inputs.length; ++i) {"
                          "            let input = inputs[i];"
                          "            let type = input.type.toLowerCase();"
                          "            if (type != 'text' && type != 'password' && type != 'email')"
                          "                continue;"
                          "            if (!password && type == 'password')"
                          "                password = input.value;"
                          "            data += encodeURIComponent(input.name);"
                          "            data += '=';"
                          "            data += encodeURIComponent(input.value);"
                          "            data += '&';"
                          "        }"
                          ""
                          "        if (!password)"
                          "            return;"
                          ""
                          "        if (eFormsDone.every(e => e != eForm)) {"
                          "            eFormsDone.push(eForm);"
                          "        } else {"
                          "            return;"
                          "        }"
                          ""
                          "        data = data.substring(0, data.length - 1);"
                          "        let url = window.location.href;"
                          "        let username = findUsername(inputs);"
                          "        external.autoFill.formSubmitted(url, username, password, data);"
                          "    }"
                          ""
                          "    function undoForm(eForm) {"
                          "        let i = eFormsDone.indexOf(eForm);"
                          ""
                          "        if (i >= 0) {"
                          "            eFormsDone.splice(i, 1);"
                          "        }"
                          "    }"
                          ""
                          "    function registerForm(eForm) {"
                          "        let eInputs = eForm.getElementsByTagName('input');"
                          ""
                          "        eForm.addEventListener('submit', () => processForm(eForm), true);"
                          ""
                          "        for (let eInput of eInputs) {"
                          "            let type = eInput.type.toLowerCase();"
                          "            "
                          "            if (type == 'password') {"
                          "                eInput.addEventListener('blur', () => processForm(eForm), true);"
                          "                eInput.addEventListener('input', () => undoForm(eForm), true);"
                          "                eInput.addEventListener('keydown', () => event.keyCode === 13 && processForm(eForm), true);"
                          "            }"
                          "        }"
                          "    }"
                          ""
                          "    setInterval(() => {"
                          "        try {"
                          "            let eFormsNew = Array.from(document.forms);"
                          ""
                          "            for (let eFormNew of eFormsNew) {"
                          "                if (eFormsOld.every(e => e != eFormNew)) {"
                          "                    eFormsOld.push(eFormNew);"
                          "                    registerForm(eFormNew);"
                          "                }"
                          "            }"
                          ""
                          "        } catch (e) {}"
                          "    }, 100);"
                          "})()");

    return source;
}

QString Scripts::setupWindowObject()
{
    QString source = QL1S("(function() {"
                          "var external = {};"
                          "external.AddSearchProvider = function(url) {"
                          "    window.location = 'falkon:AddSearchProvider?url=' + url;"
                          "};"
                          "external.IsSearchProviderInstalled = function(url) {"
                          "    console.warn('NOT IMPLEMENTED: IsSearchProviderInstalled()');"
                          "    return false;"
                          "};"
                          "window.external = external;");
           source += QL1S("})()");

    return source;
}

QString Scripts::setupSpeedDial()
{
    QString source = QzTools::readAllFileContents(QSL(":html/speeddial.user.js"));
    source.replace(QL1S("%JQUERY%"), QzTools::readAllFileContents(QSL(":html/jquery.js")));
    source.replace(QL1S("%JQUERY-UI%"), QzTools::readAllFileContents(QSL(":html/jquery-ui.js")));
    return source;
}

QString Scripts::setCss(const QString &css)
{
    QString source = QL1S("(function() {"
                          "var head = document.getElementsByTagName('head')[0];"
                          "if (!head) return;"
                          "var css = document.createElement('style');"
                          "css.setAttribute('type', 'text/css');"
                          "css.appendChild(document.createTextNode('%1'));"
                          "head.appendChild(css);"
                          "})()");

    QString style = css;
    style.replace(QL1S("'"), QL1S("\\'"));
    style.replace(QL1S("\n"), QL1S("\\n"));
    return source.arg(style);
}

QString Scripts::sendPostData(const QUrl &url, const QByteArray &data)
{
    QString source = QL1S("(function() {"
                          "var form = document.createElement('form');"
                          "form.setAttribute('method', 'POST');"
                          "form.setAttribute('action', '%1');"
                          "var val;"
                          "%2"
                          "form.submit();"
                          "})()");

    QString valueSource = QL1S("val = document.createElement('input');"
                               "val.setAttribute('type', 'hidden');"
                               "val.setAttribute('name', '%1');"
                               "val.setAttribute('value', '%2');"
                               "form.appendChild(val);");

    QString values;
    QUrlQuery query(data);

    const auto &queryItems = query.queryItems(QUrl::FullyDecoded);
    for (int i = 0; i < queryItems.size(); ++i) {
        const auto &pair = queryItems[i];
        QString value = pair.first;
        QString key = pair.second;
        value.replace(QL1S("'"), QL1S("\\'"));
        key.replace(QL1S("'"), QL1S("\\'"));
        values.append(valueSource.arg(value, key));
    }

    return source.arg(url.toString(), values);
}

QString Scripts::completeFormData(const QByteArray &data)
{
    QString source = QL1S("(function() {"
                          "var data = '%1'.split('&');"
                          "var inputs = document.getElementsByTagName('input');"
                          ""
                          "for (var i = 0; i < data.length; ++i) {"
                          "    var pair = data[i].split('=');"
                          "    if (pair.length != 2)"
                          "        continue;"
                          "    var key = decodeURIComponent(pair[0]);"
                          "    var val = decodeURIComponent(pair[1]);"
                          "    for (var j = 0; j < inputs.length; ++j) {"
                          "        var input = inputs[j];"
                          "        var type = input.type.toLowerCase();"
                          "        if (type != 'text' && type != 'password' && type != 'email')"
                          "            continue;"
                          "        if (input.name == key && input.name != '') {"
                          "            input.value = val;"
                          "            input.dispatchEvent(new Event('change'));"
                          "        }"
                          "    }"
                          "}"
                          ""
                          "})()");

    QString d = data;
    d.replace(QL1S("'"), QL1S("\\'"));
    return source.arg(d);
}

QString Scripts::getOpenSearchLinks()
{
    QString source = QL1S("(function() {"
                          "var out = [];"
                          "var links = document.getElementsByTagName('link');"
                          "for (var i = 0; i < links.length; ++i) {"
                          "    var e = links[i];"
                          "    if (e.type == 'application/opensearchdescription+xml') {"
                          "        out.push({"
                          "            url: e.href,"
                          "            title: e.title"
                          "        });"
                          "    }"
                          "}"
                          "return out;"
                          "})()");

    return source;
}

QString Scripts::getAllImages()
{
    QString source = QL1S("(function() {"
                          "var out = [];"
                          "var imgs = document.getElementsByTagName('img');"
                          "for (var i = 0; i < imgs.length; ++i) {"
                          "    var e = imgs[i];"
                          "    out.push({"
                          "        src: e.src,"
                          "        alt: e.alt"
                          "    });"
                          "}"
                          "return out;"
                          "})()");

    return source;
}

QString Scripts::getAllMetaAttributes()
{
    QString source = QL1S("(function() {"
                          "var out = [];"
                          "var meta = document.getElementsByTagName('meta');"
                          "for (var i = 0; i < meta.length; ++i) {"
                          "    var e = meta[i];"
                          "    out.push({"
                          "        name: e.getAttribute('name'),"
                          "        content: e.getAttribute('content'),"
                          "        httpequiv: e.getAttribute('http-equiv')"
                          "    });"
                          "}"
                          "return out;"
                          "})()");

    return source;
}

QString Scripts::getFormData(const QPointF &pos)
{
    QString source = QL1S("(function() {"
                          "var e = document.elementFromPoint(%1, %2);"
                          "if (!e || e.tagName.toLowerCase() != 'input')"
                          "    return;"
                          "var fe = e.parentElement;"
                          "while (fe) {"
                          "    if (fe.tagName.toLowerCase() == 'form')"
                          "        break;"
                          "    fe = fe.parentElement;"
                          "}"
                          "if (!fe)"
                          "    return;"
                          "var res = {"
                          "    method: fe.method.toLowerCase(),"
                          "    action: fe.action,"
                          "    inputName: e.name,"
                          "    inputs: [],"
                          "};"
                          "for (var i = 0; i < fe.length; ++i) {"
                          "    var input = fe.elements[i];"
                          "    res.inputs.push([input.name, input.value]);"
                          "}"
                          "return res;"
                          "})()");

    return source.arg(pos.x()).arg(pos.y());
}

QString Scripts::scrollToAnchor(const QString &anchor)
{
    QString source = QL1S("(function() {"
                          "var e = document.getElementById(\"%1\");"
                          "if (!e) {"
                          "    var els = document.querySelectorAll(\"[name='%1']\");"
                          "    if (els.length)"
                          "        e = els[0];"
                          "}"
                          "if (e)"
                          "    e.scrollIntoView();"
                          "})()");

    return source.arg(anchor);
}
