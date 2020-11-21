#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>
#include <QtWebSockets/QtWebSockets>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void extensionSocket(const QString& title);
private:
    Ui::MainWindow* ui;
    std::unique_ptr<QWebSocket> m_socket;
};
#endif // MAINWINDOW_H
