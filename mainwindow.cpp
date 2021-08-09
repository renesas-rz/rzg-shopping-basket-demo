/*****************************************************************************************
 * Copyright (C) 2021 Renesas Electronics Corp.
 * This file is part of the RZG Shopping Basket Demo.
 *
 * The RZG Shopping Basket Demo is free software using the Qt Open Source Model: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * The RZG Shopping Basket Demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the RZG Shopping Basket Demo.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************************/

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QImageReader>
#include <QElapsedTimer>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tfliteworker.h"
#include "opencvworker.h"

const QStringList MainWindow::labelList = {"Baked Beans", "Coke", "Diet Coke",
                               "Fusilli Pasta", "Lindt Chocolate",
                               "Mars", "Penne Pasta", "Pringles",
                               "Redbull", "Sweetcorn"};

const std::vector<float> MainWindow::costs = {float(0.85), float(0.82),
                                              float(0.79), float(0.89),
                                              float(1.80), float(0.80),
                                              float(0.89), float(0.85),
                                              float(1.20), float(0.69)};

MainWindow::MainWindow(QWidget *parent, QString cameraLocation, QString modelLocation)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    webcamName = cameraLocation;
    ui->setupUi(this);
    this->resize(MAINWINDOW_WIDTH, MAINWINDOW_HEIGHT);
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->tableWidget->setHorizontalHeaderLabels({"Item", "Price"});
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->setColumnWidth(0, TABLE_COLUMN_WIDTH);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    ui->checkBoxContinuous->setEnabled(false);
    ui->pushButtonWebcam->setEnabled(false);
    ui->pushButtonCapture->setEnabled(false);

    qRegisterMetaType<cv::Mat>();
    opencvThread = new QThread();
    opencvThread->setObjectName("opencvThread");
    opencvThread->start();
    cvWorker = new opencvWorker();
    cvWorker->moveToThread(opencvThread);

    connect(ui->pushButtonWebcam, SIGNAL(toggled(bool)), this,
            SLOT(pushButtonWebcamCheck(bool)));
    connect(cvWorker, SIGNAL(sendImage(const QImage&)), this, SLOT(showImage(const QImage&)));
    connect(cvWorker, SIGNAL(webcamInit(bool)), this, SLOT(webcamInitStatus(bool)));

    QMetaObject::invokeMethod(cvWorker, "initialiseWebcam", Qt::AutoConnection, Q_ARG(QString, webcamName));

    qRegisterMetaType<QVector<float> >("QVector<float>");
    tfliteThread = new QThread();
    tfliteThread->setObjectName("tfliteThread");
    tfliteThread->start();
    tfWorker = new tfliteWorker(modelLocation);
    tfWorker->moveToThread(tfliteThread);

    connect(tfWorker, SIGNAL(requestImage()), this, SLOT(receiveRequest()));
    connect(this, SIGNAL(sendImage(const QImage&)), tfWorker, SLOT(receiveImage(const QImage&)));
    connect(tfWorker, SIGNAL(sendOutputTensor(const QVector<float>&, int, const QImage&)),
            this, SLOT(receiveOutputTensor(const QVector<float>&, int, const QImage&)));
    connect(this, SIGNAL(sendNumOfInferenceThreads(int)), tfWorker, SLOT(receiveNumOfInferenceThreads(int)));

    fpsTimer = new QElapsedTimer();
}

void MainWindow::on_pushButtonImage_clicked()
{
    qeventLoop = new QEventLoop;
    QString fileName;
    QStringList fileNames;
    QFileDialog dialog(this);
    QString imageFilter;

    connect(this, SIGNAL(imageLoaded()), qeventLoop, SLOT(quit()));

    on_pushButtonStop_clicked();
    ui->pushButtonWebcam->setChecked(false);
    outputTensor.clear();
    ui->tableWidget->setRowCount(0);
    ui->labelInferenceTime->clear();

    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(IMAGE_DIRECTORY);

    imageFilter = "Images (";
    for (int i = 0; i < QImageReader::supportedImageFormats().count(); i++)
        imageFilter += "*." + QImageReader::supportedImageFormats().at(i) + " ";

    imageFilter +=")";

    dialog.setNameFilter(imageFilter);
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    if(fileNames.count() > 0)
        fileName = fileNames.at(0);

    if (!fileName.trimmed().isEmpty()) {
        imageToSend.load(fileName);
        if (imageToSend.width() != IMAGE_WIDTH || imageToSend.height() != IMAGE_HEIGHT)
            imageToSend = imageToSend.scaled(IMAGE_WIDTH, IMAGE_HEIGHT);

        image = QPixmap::fromImage(imageToSend);
        scene->clear();
        scene->addPixmap(image);
        scene->setSceneRect(image.rect());
    }

    emit imageLoaded();
    qeventLoop->exec();
}

void MainWindow::on_pushButtonRun_clicked()
{
    if (!(image.depth() > 0)) {
        QMessageBox *msgBox = new QMessageBox(QMessageBox::Warning, "Warning", "No source selected, please select an image.", QMessageBox::NoButton, this, Qt::Dialog | Qt::ToolTip);
        msgBox->show();
        return;
    }

    QMetaObject::invokeMethod(tfWorker, "process");
    ui->pushButtonRun->setEnabled(false);
}

void MainWindow::on_inferenceThreadCount_valueChanged(int threads)
{
    emit sendNumOfInferenceThreads(threads);
}

void MainWindow::receiveRequest()
{
    sendImage(imageToSend);
    tfWorker->moveToThread(tfliteThread);
}

void MainWindow::receiveOutputTensor(const QVector<float>& receivedTensor, int receivedTimeElapsed, const QImage& receivedImage)
{
    if (ui->pushButtonRun->isEnabled())
        return;

    QTableWidgetItem* item;
    float totalCost = 0;

    outputTensor = receivedTensor;
    ui->tableWidget->setRowCount(0);
    labelListSorted.clear();

    for (int i = 0; (i + 5) < receivedTensor.size(); i += 6) {
        totalCost += costs[int(outputTensor[i])];
        labelListSorted.push_back(labelList[int(outputTensor[i])]);
    }

    labelListSorted.sort();

    for (int i = 0; i < labelListSorted.size(); i++) {
        QTableWidgetItem* item = new QTableWidgetItem(labelListSorted.at(i));
        item->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->insertRow(ui->tableWidget->rowCount());
        ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, item);
        ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1,
        new QTableWidgetItem("£" + QString::number(
            double(costs[labelList.indexOf(labelListSorted.at(i))]), 'f', 2)));
    }

    ui->labelInferenceTime->setText(QString("%1 ms").arg(receivedTimeElapsed));
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());

    item = new QTableWidgetItem("Total Cost");
    item->setTextAlignment(Qt::AlignBottom | Qt::AlignRight);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, item);

    item = new QTableWidgetItem("£" + QString::number(double(totalCost), 'f', 2));
    item->setTextAlignment(Qt::AlignBottom);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1, item);

    if (!ui->checkBoxContinuous->isChecked()) {
        ui->pushButtonRun->setEnabled(true);
    } else {
        image = QPixmap::fromImage(receivedImage);
        scene->clear();
        image.scaled(ui->graphicsView->width(), ui->graphicsView->height(), Qt::KeepAspectRatio);
        drawFPS(fpsTimer->restart());
        scene->addPixmap(image);
        scene->setSceneRect(image.rect());
        QMetaObject::invokeMethod(tfWorker, "process");
    }

    drawBoxes();
}

void MainWindow::on_pushButtonStop_clicked()
{
    ui->pushButtonRun->setEnabled(true);
}

void MainWindow::on_pushButtonCapture_clicked()
{
    on_pushButtonStop_clicked();
    ui->pushButtonWebcam->setChecked(false);
    outputTensor.clear();
    ui->tableWidget->setRowCount(0);
    ui->labelInferenceTime->clear();

    imageToSend = imageNew;
    image = QPixmap::fromImage(imageToSend);
    scene->clear();
    scene->addPixmap(image);
    scene->setSceneRect(image.rect());
}

void MainWindow::on_pushButtonWebcam_clicked()
{
    outputTensor.clear();
    ui->tableWidget->setRowCount(0);
    ui->labelInferenceTime->clear();
    fpsTimer->start();
    if (ui->pushButtonWebcam->isChecked())
        QMetaObject::invokeMethod(cvWorker, "readFrame");
}

void MainWindow::showImage(const QImage& imageToShow)
{
    if (ui->pushButtonWebcam->isEnabled())
        QMetaObject::invokeMethod(cvWorker, "readFrame");

    imageNew = imageToShow;

    if ((imageNew.width() != IMAGE_WIDTH || imageNew.height() != IMAGE_HEIGHT) && imageNew.depth() > 0)
        imageNew = imageNew.scaled(IMAGE_WIDTH, IMAGE_HEIGHT);

    if (ui->pushButtonWebcam->isChecked())
        imageToSend = imageNew;

    if (ui->pushButtonWebcam->isChecked() && !ui->checkBoxContinuous->isChecked()) {
        image = QPixmap::fromImage(imageToSend);
        scene->clear();
        scene->addPixmap(image);
        scene->setSceneRect(image.rect());
        drawBoxes();
    }
}

void MainWindow::drawBoxes()
{
    for (int i = 0; (i + 5) < outputTensor.size(); i += 6) {
        QPen pen;
        QBrush brush;
        QGraphicsTextItem* itemName = scene->addText(nullptr);
        float ymin = outputTensor[i + 2] * float(scene->height());
        float xmin = outputTensor[i + 3] * float(scene->width());
        float ymax = outputTensor[i + 4] * float(scene->height());
        float xmax = outputTensor[i + 5] * float(scene->width());
        float scorePercentage = outputTensor[i + 1] * 100;

        pen.setColor(BOX_COLOUR);
        pen.setWidth(BOX_WIDTH);

        itemName->setHtml(QString("<div style='background:rgba(0, 0, 0, 100%);'>" +
                                  QString(labelList[int(outputTensor[i])] + " " +
                                  QString::number(double(scorePercentage), 'f', 1) + "%") +
                                  QString("</div>")));
        itemName->setPos(double(xmin - X_TEXT_OFFSET), double(ymin - Y_TEXT_OFFSET));
        itemName->setDefaultTextColor(TEXT_COLOUR);
        itemName->setZValue(1);

        scene->addRect(double(xmin), double(ymin), double(xmax - xmin), double(ymax - ymin), pen, brush);
    }
}

void MainWindow::drawFPS(qint64 timeElapsed)
{
    float fpsValue = 1000.0/timeElapsed;
    QGraphicsTextItem* itemFPS = scene->addText(nullptr);
    itemFPS->setHtml(QString("<div style='background:rgba(0, 0, 0, 100%);font-size:x-large;'>" +
                      QString(QString::number(double(fpsValue), 'f', 1) + " FPS") +
                      QString("</div>")));
    itemFPS->setPos(scene->width() - X_FPS , Y_FPS);
    itemFPS->setDefaultTextColor(TEXT_COLOUR);
    itemFPS->setZValue(1);
}

/*
 * If the webcam button is clicked
 * and the webcam button is checked (pressed down),
 * then enable the webcam continuous checkbox.
 */
void MainWindow::pushButtonWebcamCheck(bool webcamButtonChecked)
{
    if (webcamButtonChecked) {
        ui->checkBoxContinuous->setEnabled(true);
    } else {
        ui->checkBoxContinuous->setCheckState(Qt::Unchecked);
        ui->checkBoxContinuous->setEnabled(false);
    }
}

void MainWindow::webcamInitStatus(bool webcamStatus)
{
    if (!webcamStatus) {
        webcamNotConnected();
        ui->pushButtonWebcam->setChecked(false);
    } else {
        ui->pushButtonWebcam->setEnabled(true);
        ui->pushButtonCapture->setEnabled(true);
        cvWorker->checkWebcam();
    }
}

void MainWindow::on_actionLicense_triggered()
{
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "License",
                             "Copyright (C) 2021 Renesas Electronics Corp.\n\n"
                             "The RZG Shopping Basket Demo is free software using the Qt Open Source Model: "
                             "you can redistribute it and/or modify "
                             "it under the terms of the GNU General Public License as published by "
                             "the Free Software Foundation, either version 2 of the License, or "
                             "(at your option) any later version.\n\n"
                             "The RZG Shopping Basket Demo is distributed in the hope that it will be useful, "
                             "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                             "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                             "GNU General Public License for more details.\n\n"
                             "You should have received a copy of the GNU General Public License "
                             "along with the RZG Shopping Basket Demo. If not, see https://www.gnu.org/licenses."
                             , QMessageBox::NoButton, this, Qt::Dialog | Qt::ToolTip);
    msgBox->show();
}

void MainWindow::on_actionReset_triggered()
{
    QMetaObject::invokeMethod(cvWorker, "initialiseWebcam", Qt::AutoConnection, Q_ARG(QString,webcamName));
}

void MainWindow::webcamNotConnected()
{
    opencvThread->deleteLater();
    ui->pushButtonWebcam->setEnabled(false);
    ui->pushButtonCapture->setEnabled(false);
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Warning, "Warning", "Webcam not connected", QMessageBox::NoButton, this, Qt::Dialog | Qt::ToolTip);
    msgBox->show();
    ui->pushButtonWebcam->setChecked(false);
}

void MainWindow::on_actionDisconnect_triggered()
{
    QMetaObject::invokeMethod(cvWorker, "disconnectWebcam");
    webcamNotConnected();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}
