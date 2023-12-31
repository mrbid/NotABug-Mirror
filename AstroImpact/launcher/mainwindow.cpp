#include <QSettings>
#include <unistd.h>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    ui->epoch->setText(QString::number(((((time_t)(((double)time(0) / 60.0) / 3.0)+1)*3)*60)));
    ui->msaa->setValue(16);
    ui->asteroids->setValue(64);

    QSettings settings;
    settings.beginGroup(this->objectName());
    QVariant center = settings.value("center");
    if(!center.isNull())
    {
        ui->center->setCheckState(Qt::CheckState(center.toInt()));
        ui->tilt->setCheckState(Qt::CheckState(settings.value("tilt").toInt()));
        ui->msaa->setValue(settings.value("msaa").toInt());
        ui->asteroids->setValue(settings.value("asteroids").toInt());
        ui->hostname->setText(settings.value("hostname").toString());
    }
    settings.endGroup();

    timerId = startTimer(1);
    ui->join->setFocus();
}

MainWindow::~MainWindow()
{
    killTimer(timerId);
    QSettings settings;
    settings.beginGroup(this->objectName());
    settings.setValue("center", QString::number(ui->center->checkState()));
    settings.setValue("tilt", QString::number(ui->tilt->checkState()));
    settings.setValue("msaa", QString::number(ui->msaa->value()));
    settings.setValue("asteroids", QString::number(ui->asteroids->value()));
    settings.setValue("hostname", ui->hostname->text());
    settings.endGroup();
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == timerId)
    {
        long long eta = ui->epoch->text().toULong()-time(0);
        ui->eta->setText("ETA " + QString::number(eta) + "s");
        if(eta <= 33)
        {
            ui->eta->setStyleSheet("color:#dc3545");
            if(eta <= 0){ui->eta->setText("Missed Epoch");}
        }
        else
        {
            ui->eta->setStyleSheet("color:#20c997");
            if(eta >= 1800){ui->eta->setText("A long time");}
        }
    }
}

void MainWindow::on_quit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_join_clicked()
{
    QSettings settings;
    settings.beginGroup(this->objectName());
    settings.setValue("center", QString::number(ui->center->checkState()));
    settings.setValue("tilt", QString::number(ui->tilt->checkState()));
    settings.setValue("msaa", QString::number(ui->msaa->value()));
    settings.setValue("asteroids", QString::number(ui->asteroids->value()));
    settings.setValue("hostname", ui->hostname->text());
    settings.endGroup();
    settings.sync();

    execl("/usr/games/fat", "/usr/games/fat", ui->epoch->text().toStdString().c_str(), ui->hostname->text().toStdString().c_str(), QString::number(ui->asteroids->value()).toStdString().c_str(), QString::number(ui->msaa->value()).toStdString().c_str(), QString::number(ui->tilt->isChecked()).toStdString().c_str(), QString::number(ui->center->isChecked()).toStdString().c_str(), (char*)NULL);
}


void MainWindow::on_reset_clicked()
{
    ui->epoch->setText(QString::number(((((time_t)(((double)time(0) / 60.0) / 3.0)+1)*3)*60)));
    ui->msaa->setValue(16);
    ui->asteroids->setValue(64);
    ui->center->setCheckState(Qt::CheckState(2));
    ui->tilt->setCheckState(Qt::CheckState(2));
    ui->hostname->setText("vfcash.co.uk");
}


void MainWindow::on_maxast_clicked()
{
    if(ui->asteroids->value() == 65535)
        ui->asteroids->setValue(16);
    else
        ui->asteroids->setValue(65535);
}


void MainWindow::on_newepoch_clicked()
{
    ui->epoch->setText(QString::number(time(0)+180));
}

