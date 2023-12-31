#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_quit_clicked();

    void on_join_clicked();

    void on_reset_clicked();

    void on_maxast_clicked();

    void on_newepoch_clicked();

private:
    Ui::MainWindow *ui;
    void timerEvent(QTimerEvent *event);
    int timerId;
};
#endif // MAINWINDOW_H
