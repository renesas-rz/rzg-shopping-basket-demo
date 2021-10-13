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

#ifndef TFLITEWORKER_H
#define TFLITEWORKER_H

#include <tensorflow/lite/kernels/register.h>

#include <QObject>
#include <QVector>

#include <opencv2/videoio.hpp>

#define DETECT_THRESHOLD 0.5

class tfliteWorker : public QObject
{
    Q_OBJECT

public:
    tfliteWorker(QString modelLocation, bool armnnDelegate, int defaultThreads);
    void receiveImage(const cv::Mat&);

signals:
    void sendOutputTensor(const QVector<float>&, int, const cv::Mat&);

private:
    std::unique_ptr<tflite::Interpreter> tfliteInterpreter;
    std::unique_ptr<tflite::FlatBufferModel> tfliteModel;
    std::string modelName;
    QVector<float> outputTensor;
    int wantedWidth, wantedHeight, wantedChannels;
};

#endif // TFLITEWORKER_H
