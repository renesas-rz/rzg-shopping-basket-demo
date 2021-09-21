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

#define CPU_MODEL_NAME "shoppingBasketDemo.tflite"

#define G2L_HW_INFO "Hardware Information\n\nBoard: RZ/G2L smarc-rzg2l-evk\nCPUs: 2x Arm Cortex-A55\nDDR: 2GB"
#define G2M_HW_INFO "Hardware Information\n\nBoard: RZ/G2M hihope-rzg2m\nCPUs: 2x Arm Cortex-A57, 4x Arm Cortex-A53\nDDR: 4GB"
#define HW_INFO_WARNING "Unknown Board!"
#define IMAGE_DIRECTORY "sample_images"
#define TEXT_INFERENCE "Inference Time: "
#define TEXT_FPS "Total FPS: "
#define TEXT_TOTAL_ITEMS "Total Items: "

#define TABLE_COLUMN_WIDTH 180
#define GRAPHICS_VIEW_WIDTH_4K 700 //This is for the 4k resolution
#define BOX_WIDTH 2
#define BOX_COLOUR Qt::green
#define TEXT_COLOUR Qt::green

class QGraphicsScene;
class QGraphicsView;
class QEventLoop;
class opencvWorker;
class tfliteWorker;
class QElapsedTimer;

namespace Ui { class MainWindow; } //Needed for mainwindow.ui

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, QString cameraLocation, QString modelLocation);

signals:
    void sendImage(const cv::Mat&);
    void sendNumOfInferenceThreads(int threads);
    void imageLoaded();
    void sendInitWebcam();
    void sendReadFrame();
    void processImage();

public slots:
    void receiveRequest();
    void showImage(const cv::Mat& matToShow);
    void drawMatToView(const cv::Mat& matInput);
    void on_actionExit_triggered();

private slots:
    void receiveOutputTensor (const QVector<float>& receivedTensor, int recievedTimeElapsed, const cv::Mat&);
    void webcamInitStatus (bool webcamStatus);
    void on_pushButtonProcessBasket_clicked();
    void on_actionLicense_triggered();
    void on_actionReset_triggered();
    void on_actionEnable_ArmNN_Delegate_triggered();
    void webcamNotConnected();
    void setImageSize();
    void on_actionDisconnect_triggered();
    void on_actionHardware_triggered();

private:
    void drawBoxes();
    void drawFPS(qint64 timeElapsed);
    void createTfThread();
    void setApplicationSize();
    QImage matToQImage(const cv::Mat& matToConvert);

    Ui::MainWindow *ui;
    bool useArmNNDelegate;
    QPixmap image;
    QGraphicsScene *scene;
    QImage imageNew;
    QImage imageToSend;
    QVector<float> outputTensor;
    QGraphicsView *graphicsView;
    opencvWorker *cvWorker;
    tfliteWorker *tfWorker;
    QThread *opencvThread, *tfliteThread;
    QEventLoop *qeventLoop;
    QStringList labelListSorted;
    QString boardInfo;
    QString webcamName;
    QString modelPath;
    static const QStringList labelList;
    static const std::vector<float> costs;
    QElapsedTimer *fpsTimer;
    int imageWidth;
    int imageHeight;
    bool webcamDisconnect;
    cv::Mat matToSend;
    cv::Mat matNew;
};

#endif // MAINWINDOW_H
