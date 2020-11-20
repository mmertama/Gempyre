#include <QtWebEngineWidgets/QWebEngineView>
#include <QLayout>
#include <QGuiApplication>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_socket(new QWebSocket()) {



    ui->setupUi(this);
    auto *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    auto view = new QWebEngineView();
    layout->addWidget(view);

    auto width = 1024;
    auto height = 768;
    if(QCoreApplication::arguments().length() > 2) {
        bool ok;
        const auto w =  QCoreApplication::arguments().at(2).toInt(&ok);
        if(ok) {
            width = w;
            setFixedWidth(w);
        }
    }

    if(QCoreApplication::arguments().length() > 3) {
        bool ok;
        const auto h =  QCoreApplication::arguments().at(3).toInt(&ok);
        if(ok) {
            height = h;
            setFixedHeight(h);
        }
    }

    QString title = "Gempyre client";

    if(QCoreApplication::arguments().length() > 4) {
         title = QCoreApplication::arguments().at(4);
    }

    QGuiApplication::setApplicationDisplayName(title);
    extensionSocket(title);

    if(QCoreApplication::arguments().length() > 1) {
        const auto uri =  QUrl(QCoreApplication::arguments().at(1));
        if(!uri.isEmpty()) {
            QObject::connect(view, &QWebEngineView::loadFinished, [this, uri](bool){
                const auto wsuri = QString("ws://%1:%2/gempyre").arg(uri.host()).arg(uri.port());
                m_socket->open(wsuri);
            });
            view->load(uri);
        }
        else {
            qCritical() << "Invalid URL" << uri;
            exit(-2);
        }
    } else {
        qCritical() << "Usage: URL <width> <height> <title>";
        exit(-1);
    }


    centralWidget()->setLayout(layout);
    centralWidget()->resize(width, height);

    view->show();
}

MainWindow::~MainWindow() {
    delete ui;
}


static QString makeFilters(const QJsonObject& filters) {
    QStringList filterList;
    for(const auto& f : filters) {
        const auto fe = f.toObject();
        QString filter;
        if(fe.contains("title")) {
            const auto title = fe["title"].toString();
            filter.append(title);
        }
        filter.append('(');
        const auto exts = f["filters"].toArray();
        QStringList es;
        std::transform(exts.begin(), exts.end(), std::back_inserter(es), [](const auto& s){return s.toString();});
        filter.append(es.join(' '));
        filter.append(')');
        filterList.append(filter);
    }
    return filterList.join(";;");
}

void MainWindow::extensionSocket(const QString& title) {
    QObject::connect(m_socket.get(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [this, title] (QAbstractSocket::SocketError err) {
        //if(error != QWebSocket::Clos)
        QMessageBox::warning(this, title,
                                      QString("%1 (%2)").arg(m_socket->errorString()).arg(err),
                                       QMessageBox::Ok);
        qCritical() << "Socket error" << err << this->m_socket->errorString();
    });
    QObject::connect(m_socket.get(), &QWebSocket::binaryMessageReceived, this, [this, title](const QByteArray&) {
         QMessageBox::warning(this, title, "Not supported message", QMessageBox::Ok);
    });
    QObject::connect(m_socket.get(), &QWebSocket::connected, this, [this, title]() {
        m_socket->sendTextMessage(QJsonDocument(QJsonObject({
                                                            {"type", "extensionready"}})).toJson());
    });
    QObject::connect(m_socket.get(), &QWebSocket::textMessageReceived, this, [this, title](const QString& message) {
            const auto doc = QJsonDocument::fromJson(message.toUtf8());
            if(!doc.isObject()) {
                QMessageBox::warning(this, title, "Message is not an object", QMessageBox::Ok);
                qWarning() << "Cannot read message" << message;
                qWarning() << "expect type:openFile|openFiles|openDir|saveFile" << message;
                qWarning() << "having, caption, dir, filter, id fields";
                qWarning() << "caption, dir, filter, id fields";
                qWarning() << "caption, title of dialog";
                qWarning() << "dir, initial directory";
                qWarning() << "id, some unique value in current app context";
                qWarning() << "filter, array of objects contains title, filters";
                qWarning() << "title of filter";
                qWarning() << "filters, array of extension filtters (e.g. *.jpg)";
            }
            const auto obj = doc.object();
            if(obj["type"] != "extension") {
                return;
            }

            const auto callid = obj["extension_call"].toString();
            const auto params = obj["extension_parameters"].toObject();
            const auto id = obj["extension_id"].toString();
            if(callid == "openFile") {
                const auto file = QFileDialog::getOpenFileName(this,
                        params["caption"].toString(),
                        params["dir"].toString(),
                        makeFilters(params["filter"].toObject()));
                m_socket->sendTextMessage(QJsonDocument(QJsonObject({
                                                                    {"type", "extension_response"},
                                                                    {"extension_call", "openFileResponse"},
                                                                    {"extension_id", id},
                                                                    {"openFileResponse", file}})).toJson());
            }
            else if(callid == "openFiles") {
                const auto files = QFileDialog::getOpenFileNames(this,
                        params["caption"].toString(),
                        params["dir"].toString(),
                        makeFilters(params["filter"].toObject()));
                QJsonArray array;
                std::transform(files.begin(), files.end(), std::back_inserter(array), [](const auto& s){return s;});
                m_socket->sendTextMessage(QJsonDocument(QJsonObject({
                                                                    {"type", "extension_response"},
                                                                    {"extension_call", "openFilesResponse"},
                                                                    {"extension_id", id},
                                                                    {"openFilesResponse", array}})).toJson());
            }
            else if(callid == "openDir") {
                const auto dir = QFileDialog::getExistingDirectory(this,
                        params["caption"].toString(),
                        params["dir"].toString());
                m_socket->sendTextMessage(QJsonDocument(QJsonObject({
                                                                    {"type", "extension_response"},
                                                                    {"extension_call", "openDirResponse"},
                                                                    {"id", id},
                                                                    {"openDirResponse", dir}})).toJson());
            }
            else if(callid == "saveFile") {
                const auto file = QFileDialog::getSaveFileName(this,
                        params["caption"].toString(),
                        params["dir"].toString(),
                        makeFilters(params["filter"].toObject()));
                m_socket->sendTextMessage(QJsonDocument(QJsonObject({
                                                                    {"type", "extension_response"},
                                                                    {"extension_call", "saveFileResponse"},
                                                                    {"id", id},
                                                                    {"savesFileResponse", file}})).toJson());
            }
            else {
                QMessageBox::warning(this, title, QString("\"%1\" is not valid extension requests").arg(callid), QMessageBox::Ok);
            }
    });
}

