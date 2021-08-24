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

#include <QCamera>
#include <QCameraImageCapture>
#include <QImage>

#include "opencvworker.h"

void opencvWorker::initialiseWebcam(QString cameraLocation)
{
    QString gstreamerPipeline;

    getResolution();
    if (webcamInitialised == true)
        videoCapture->release();

    if (!cameraLocation.isEmpty()) {
        gstreamerPipeline = "v4l2src device=" + QString(cameraLocation) +
            " ! video/x-raw, width=" + QString::number(cameraWidth) + ", height=" + QString::number(cameraHeight) +
            " ! videoconvert ! appsink";

        videoCapture.reset(new cv::VideoCapture(gstreamerPipeline.toStdString(), cv::CAP_GSTREAMER));

        if (videoCapture->isOpened())
            webcamInitialised = true;
        else
            webcamInitialised = false;

    } else {
        webcamInitialised = false;
    }

    emit webcamInit(webcamInitialised);
}

void opencvWorker::getResolution()
{
    QList<QByteArray> cameraDevices = QCamera::availableDevices();
    QList<QSize> resolutions;
    QCamera *camera;
    QCameraImageCapture *imageCapture;

    if (cameraDevices.count() > 0) {
        QByteArray cameraDevice = cameraDevices.first();
        camera = new QCamera(cameraDevice);
        camera->load();
        imageCapture = new QCameraImageCapture(camera);
        resolutions = imageCapture->supportedResolutions();

        QSize resol = resolutions.takeLast();
        cameraWidth = resol.width();
        cameraHeight = resol.height();
    }
}

void opencvWorker::readFrame()
{
    QImage videoImage;

    if(!videoCapture->read(videoFrame)) {
            webcamInitialised = false;
            videoCapture->release();
            emit webcamInit(webcamInitialised);
            return;
    }

    videoImage = QImage(videoFrame.data, videoFrame.cols,
                     videoFrame.rows, int(videoFrame.step), QImage::Format_RGB888).rgbSwapped();
    videoFrame.release();
    emit sendImage(videoImage);
}

void opencvWorker::checkWebcam()
{
    if(!videoCapture->read(videoFrame)) {
        webcamInitialised = false;
        videoCapture->release();
        return;
    } else
         webcamInitialised = true;
}

void opencvWorker::disconnectWebcam()
{
    if (webcamInitialised == true)
        videoCapture->release();
}
