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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/videoio.hpp>

#define BUTTON_BLUE "background-color: rgba(42, 40, 157);color: rgb(255, 255, 255);border: 2px;border-radius: 55px;border-style: outset;"
#define BUTTON_GREYED_OUT "background-color: rgba(42, 40, 157, 90);color: rgb(255, 255, 255);border: 2px;border-radius: 55px;border-style: outset;"

#define TEXT_CAMERA_INIT_STATUS_ERROR "Camera Error!\n\n No camera detected, please check connection and relaunch application.\n\nApplication will now close."
#define TEXT_CAMERA_FAILURE_ERROR "Camera Error!\n\n Camera has stopped working, please check the connection and relaunch application.\n\nApplication will now close."

#define CPU_MODEL_NAME "shoppingBasketDemo.tflite"

#define G2L_HW_INFO "Hardware Information\n\nBoard: RZ/G2L smarc-rzg2l-evk\nCPUs: 2x Arm Cortex-A55\nDDR: 2GB"
#define G2M_HW_INFO "Hardware Information\n\nBoard: RZ/G2M hihope-rzg2m\nCPUs: 2x Arm Cortex-A57, 4x Arm Cortex-A53\nDDR: 4GB"
#define HW_INFO_WARNING "Unknown Board!"
#define TEXT_INFERENCE "Inference Time: "
#define TEXT_TOTAL_ITEMS "Total Items: "

#define APP_WIDTH 1275
#define APP_HEIGHT 635
#define TABLE_COLUMN_WIDTH 180
#define GRAPHICS_VIEW_EXCESS_SPACE 2 //Prevents the image from scaling outside the graphics view
#define BOX_WIDTH 2
#define BOX_COLOUR Qt::green
#define TEXT_COLOUR Qt::green
#define MIPI_VIDEO_DELAY 50

/* Application exit codes */
#define EXIT_OKAY 0
#define EXIT_CAMERA_INIT_ERROR 1
#define EXIT_CAMERA_STOPPED_ERROR 2

class QGraphicsScene;
class QGraphicsView;
class opencvWorker;
class tfliteWorker;
class QElapsedTimer;
class videoWorker;

namespace Ui { class MainWindow; } //Needed for mainwindow.ui

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, QString cameraLocation, QString modelLocation);

signals:
    void startVideo();
    void stopVideo();

public slots:
    void ShowVideo();

private slots:
    void receiveOutputTensor (const QVector<float>& receivedTensor, int recievedTimeElapsed, const cv::Mat&);
    void on_pushButtonProcessBasket_clicked();
    void on_pushButtonNextBasket_clicked();
    void on_actionLicense_triggered();
    void on_actionEnable_ArmNN_Delegate_triggered();
    void on_actionHardware_triggered();
    void on_actionExit_triggered();
    void start_video();
    void stop_video();

private:
    void drawBoxes();
    void drawFPS(qint64 timeElapsed);
    void drawMatToView(const cv::Mat& matInput);
    void createTfWorker();
    QImage matToQImage(const cv::Mat& matToConvert);
    void createVideoWorker();
    void setProcessButton(bool enable);
    void setNextButton(bool enable);

    Ui::MainWindow *ui;
    bool useArmNNDelegate;
    QPixmap image;
    QGraphicsScene *scene;
    QVector<float> outputTensor;
    QGraphicsView *graphicsView;
    opencvWorker *cvWorker;
    tfliteWorker *tfWorker;
    QStringList labelListSorted;
    QString boardInfo;
    QString modelPath;
    static const QStringList labelList;
    static const std::vector<float> costs;
    videoWorker *vidWorker;
};

#endif // MAINWINDOW_H
