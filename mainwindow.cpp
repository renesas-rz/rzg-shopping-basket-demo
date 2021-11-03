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
#include <QSysInfo>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tfliteworker.h"
#include "opencvworker.h"
#include "videoworker.h"

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
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    Board board = Unknown;

    modelPath = modelLocation;
    useArmNNDelegate = true;

    ui->setupUi(this);
    this->resize(APP_WIDTH, APP_HEIGHT);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(25);
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    font.setPointSize(14);
    ui->tableWidget->setHorizontalHeaderLabels({"Item", "Price"});
    ui->tableWidget->horizontalHeader()->setFont(font);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->resizeColumnsToContents();
    double column1Width = ui->tableWidget->geometry().width() * 0.8;
    ui->tableWidget->setColumnWidth(0, column1Width);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    ui->labelInference->setText(TEXT_INFERENCE);
    ui->labelTotalItems->setText(TEXT_TOTAL_ITEMS);

    QPixmap rzLogo;
    rzLogo.load("/opt/shopping-basket-demo/logos/renesas-rz-logo.png");
    ui->labelRzLogo->setPixmap(rzLogo);

    setProcessButton(true);
    setNextButton(false);

    qRegisterMetaType<QVector<float> >("QVector<float>");

    QSysInfo systemInfo;

    if (systemInfo.machineHostName() == "hihope-rzg2m") {
        setWindowTitle("Shopping Basket Demo - RZ/G2M");
        boardInfo = G2M_HW_INFO;
        board = G2M;

        if (cameraLocation.isEmpty() && QDir("/dev/v4l/by-id").exists())
            cameraLocation = QDir("/dev/v4l/by-id").entryInfoList(QDir::NoDotAndDotDot).at(0).absoluteFilePath();

    } else if (systemInfo.machineHostName() == "smarc-rzg2l") {
        setWindowTitle("Shopping Basket Demo - RZ/G2L");
        boardInfo = G2L_HW_INFO;
        board = G2L;

        if (cameraLocation.isEmpty())
            cameraLocation = QString("/dev/video0");

    } else if (systemInfo.machineHostName() == "ek874") {
        setWindowTitle("Shopping Basket Demo - RZ/G2E");
        boardInfo = G2E_HW_INFO;
        board = G2E;

        if (cameraLocation.isEmpty())
            cameraLocation = QString("/dev/video0");
    } else {
        setWindowTitle("Shopping Basket Demo");
        boardInfo = HW_INFO_WARNING;
    }

    qRegisterMetaType<cv::Mat>();
    cvWorker = new opencvWorker(cameraLocation, board);

    if (cvWorker->cameraInit() == false) {
        qWarning("Camera not initialising. Quitting.");
        errorPopup(TEXT_CAMERA_INIT_STATUS_ERROR, EXIT_CAMERA_INIT_ERROR);
    } else if (cvWorker->getCameraOpen() == false) {
        qWarning("Camera not opening. Quitting.");
        errorPopup(TEXT_CAMERA_OPENING_ERROR, EXIT_CAMERA_STOPPED_ERROR);
    } else {
        createVideoWorker();
        createTfWorker();

        /* Limit camera loop speed if using mipi camera to save on CPU
         * USB camera is alreay limited to 10 FPS */
        if (cvWorker->getUsingMipi())
            vidWorker->setDelayMS(MIPI_VIDEO_DELAY);

        start_video();
    }
}

void MainWindow::createVideoWorker()
{
    vidWorker = new videoWorker();

    connect(vidWorker, SIGNAL(showVideo()), this, SLOT(ShowVideo()));
    connect(this, SIGNAL(startVideo()), vidWorker, SLOT(StartVideo()));
    connect(this, SIGNAL(stopVideo()), vidWorker, SLOT(StopVideo()));
}

void MainWindow::start_video()
{
    emit startVideo();
}

void MainWindow::stop_video()
{
    emit stopVideo();
}

void MainWindow::createTfWorker()
{
    /* ArmNN Delegate sets the inference threads to amount of CPU cores
     * of the same type logically group first, which for the RZ/G2L and
     * RZ/G2M is 2 */
    int inferenceThreads = 2;
    tfWorker = new tfliteWorker(modelPath, useArmNNDelegate, inferenceThreads);

    connect(tfWorker, SIGNAL(sendOutputTensor(const QVector<float>&, int, const cv::Mat&)),
            this, SLOT(receiveOutputTensor(const QVector<float>&, int, const cv::Mat&)));
}

void MainWindow::receiveOutputTensor(const QVector<float>& receivedTensor, int receivedTimeElapsed, const cv::Mat& receivedMat)
{
    QTableWidgetItem* item;
    QTableWidgetItem* price;
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
        price = new QTableWidgetItem("£" + QString::number(
                double(costs[labelList.indexOf(labelListSorted.at(i))]), 'f', 2)));
        price->setTextAlignment(Qt::AlignRight);
    }

    ui->labelInference->setText(TEXT_INFERENCE + QString("%1 ms").arg(receivedTimeElapsed));
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());

    item = new QTableWidgetItem("Total Cost:");
    item->setTextAlignment(Qt::AlignBottom | Qt::AlignRight);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, item);

    item = new QTableWidgetItem("£" + QString::number(double(totalCost), 'f', 2));
    item->setTextAlignment(Qt::AlignBottom | Qt::AlignRight);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1, item);

    if (!ui->pushButtonProcessBasket->isEnabled())
        drawMatToView(receivedMat);

    drawBoxes();
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

        itemName->setHtml(QString("<div style='background:rgba(0, 0, 0, 100%);font-size:xx-large;'>" +
                                  QString(labelList[int(outputTensor[i])] + " " +
                                  QString::number(double(scorePercentage), 'f', 1) + "%") +
                                  QString("</div>")));
        itemName->setPos(xmin, ymin);
        itemName->setDefaultTextColor(TEXT_COLOUR);
        itemName->setZValue(1);

        scene->addRect(double(xmin), double(ymin), double(xmax - xmin), double(ymax - ymin), pen, brush);
    }
    ui->labelTotalItems->setText(TEXT_TOTAL_ITEMS + QString("%1").arg(outputTensor.size() / 6));
}

void MainWindow::on_pushButtonNextBasket_clicked()
{
    setProcessButton(true);
    setNextButton(false);

    outputTensor.clear();
    ui->tableWidget->setRowCount(0);
    ui->labelInference->setText(TEXT_INFERENCE);
    ui->labelTotalItems->setText(TEXT_TOTAL_ITEMS);

    start_video();
}

void MainWindow::ShowVideo()
{
    const cv::Mat* image;

    image = cvWorker->getImage(1);

    if (image == nullptr) {
        stop_video();
        setProcessButton(false);

        qWarning("Camera no longer working. Quitting.");
        errorPopup(TEXT_CAMERA_FAILURE_ERROR, EXIT_CAMERA_STOPPED_ERROR);
    } else {
        drawMatToView(*image);
    }
}

void MainWindow::on_pushButtonProcessBasket_clicked()
{
    const cv::Mat* image;
    unsigned int iterations;

    stop_video();

    setProcessButton(false);
    setNextButton(true);

    if (cvWorker->getUsingMipi())
        iterations = 6;
    else
        iterations = 2;

    image = cvWorker->getImage(iterations);

    if (image == nullptr) {
        setNextButton(false);

        qWarning("Camera not working. Quitting.");
        errorPopup(TEXT_CAMERA_FAILURE_ERROR, EXIT_CAMERA_STOPPED_ERROR);
    } else {
        outputTensor.clear();
        ui->tableWidget->setRowCount(0);
        ui->labelInference->setText(TEXT_INFERENCE);

        tfWorker->receiveImage(*image);
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
                             "along with the RZG Shopping Basket Demo. If not, see https://www.gnu.org/licenses.",
                             QMessageBox::NoButton, this, Qt::Dialog | Qt::FramelessWindowHint);
    msgBox->setFont(font);
    msgBox->show();
}

void MainWindow::on_actionHardware_triggered()
{
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "Information", boardInfo,
                                 QMessageBox::NoButton, this, Qt::Dialog | Qt::FramelessWindowHint);
    msgBox->setFont(font);
    msgBox->show();
}

void MainWindow::setProcessButton(bool enable)
{
    if (enable) {
        ui->pushButtonProcessBasket->setStyleSheet(BUTTON_BLUE);
        ui->pushButtonProcessBasket->setEnabled(true);
    } else {
        ui->pushButtonProcessBasket->setStyleSheet(BUTTON_GREYED_OUT);
        ui->pushButtonProcessBasket->setEnabled(false);
    }

    qApp->processEvents(QEventLoop::WaitForMoreEvents);
}

void MainWindow::setNextButton(bool enable)
{
    if (enable) {
        ui->pushButtonNextBasket->setStyleSheet(BUTTON_BLUE);
        ui->pushButtonNextBasket->setEnabled(true);
    } else {
        ui->pushButtonNextBasket->setStyleSheet(BUTTON_GREYED_OUT);
        ui->pushButtonNextBasket->setEnabled(false);
    }

    qApp->processEvents(QEventLoop::WaitForMoreEvents);
}

void MainWindow::drawMatToView(const cv::Mat& matInput)
{
    QImage imageToDraw;

    imageToDraw = matToQImage(matInput);

    image = QPixmap::fromImage(imageToDraw);
    scene->clear();
    image = image.scaled(ui->graphicsView->width() - GRAPHICS_VIEW_EXCESS_SPACE, ui->graphicsView->height() - GRAPHICS_VIEW_EXCESS_SPACE);
    scene->addPixmap(image);
    scene->setSceneRect(image.rect());
}

QImage MainWindow::matToQImage(const cv::Mat& matToConvert)
{
    QImage convertedImage;

    if (matToConvert.empty())
        return QImage(nullptr);

    convertedImage = QImage(matToConvert.data, matToConvert.cols,
                     matToConvert.rows, int(matToConvert.step),
                        QImage::Format_RGB888).copy();

    return convertedImage;
}

void MainWindow::on_actionEnable_ArmNN_Delegate_triggered()
{
    /* Update the GUI text */
    if(useArmNNDelegate) {
        ui->actionEnable_ArmNN_Delegate->setText("Enable ArmNN Delegate");
        ui->labelDelegate->setText("TensorFlow Lite");
    } else {
        ui->actionEnable_ArmNN_Delegate->setText("Disable ArmNN Delegate");
        ui->labelDelegate->setText("TensorFlow Lite + ArmNN delegate");
    }

    /* Toggle delegate state */
    useArmNNDelegate = !useArmNNDelegate;

    delete tfWorker;
    createTfWorker();
}

void MainWindow::errorPopup(QString errorMessage, int errorCode)
{
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Critical, "Error", errorMessage,
                                 QMessageBox::NoButton, this, Qt::Dialog | Qt::FramelessWindowHint);
    msgBox->setFont(font);
    msgBox->exec();

    exit(errorCode);
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}
