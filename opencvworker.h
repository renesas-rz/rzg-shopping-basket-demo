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

#ifndef OPENCVCAPTUREWORKER_H
#define OPENCVCAPTUREWORKER_H

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <memory>

#include <QObject>

#define CAMERA_WIDTH "1280"
#define CAMERA_HEIGHT "1024"


Q_DECLARE_METATYPE(cv::Mat)

class opencvWorker : public QObject
{
    Q_OBJECT

public:
    void checkWebcam();

signals:
    void sendImage(const QImage&);
    void webcamInit(bool webcamInitialised);

private:
    cv::Mat videoFrame;
    std::unique_ptr<cv::VideoCapture> videoCapture;
    bool webcamInitialised;

private slots:
    void initialiseWebcam(QString cameraLocation);
    void readFrame();
    void disconnectWebcam();
};

#endif // OPENCVCAPTUREWORKER_H
