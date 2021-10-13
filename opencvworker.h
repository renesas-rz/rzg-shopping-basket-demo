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
#include <string.h>

#include <QObject>

Q_DECLARE_METATYPE(cv::Mat)

class opencvWorker : public QObject
{
    Q_OBJECT

public:
    opencvWorker(QString cameraLocation);
    ~opencvWorker();
    cv::Mat* getImage();
    bool cameraInit();

private:
    int runCommand(std::string command, std::string &stdoutput);
    void setupCamera();

    std::unique_ptr<cv::VideoCapture> videoCapture;
    bool webcamInitialised;
    bool usingMipi;
    std::string webcamName;
    cv::Mat picture;
    cv::VideoCapture *camera;
    std::string cameraInitialization = "media-ctl -d /dev/media0 --reset && media-ctl -d /dev/media0 -l \"'rzg2l_csi2 10830400.csi2':1->'CRU output':0 [1]\" && media-ctl -d /dev/media0 -V \"'rzg2l_csi2 10830400.csi2':1 [fmt:UYVY8_2X8/1280x960 field:none]\" && media-ctl -d /dev/media0 -V \"'ov5645 0-003c':0 [fmt:UYVY8_2X8/1280x960 field:none]\"";
};

#endif // OPENCVCAPTUREWORKER_H
