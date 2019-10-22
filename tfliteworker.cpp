/*****************************************************************************************
 * Copyright (C) 2019 Renesas Electronics Corp.
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

#include <QImage>
#include <chrono>

#include "tfliteworker.h"

tfliteWorker::tfliteWorker()
{
    tflite::ops::builtin::BuiltinOpResolver tfliteResolver;
    TfLiteIntArray *wantedDimensions;

    numberOfInferenceThreads = 2;

    tfliteModel = tflite::FlatBufferModel::BuildFromFile(MODEL_NAME);
    tflite::InterpreterBuilder(*tfliteModel, tfliteResolver) (&tfliteInterpreter);

    tfliteInterpreter->AllocateTensors();
    tfliteInterpreter->SetProfiler(nullptr);
    tfliteInterpreter->SetNumThreads(numberOfInferenceThreads);

    wantedDimensions = tfliteInterpreter->tensor(tfliteInterpreter->inputs()[0])->dims;
    wantedHeight = wantedDimensions->data[1];
    wantedWidth = wantedDimensions->data[2];
    wantedChannels = wantedDimensions->data[3];
}

void tfliteWorker::process()
{
    tfliteInterpreter->SetNumThreads(numberOfInferenceThreads);
    emit requestImage();
}

/*
 * Resize the input image and manipulate the data such that the alpha channel
 * is removed. Input the data to the tensor and output the results into a vector.
 * Also measure the time it takes for this function to complete
 */
void tfliteWorker::receiveImage(const QImage& sentImage)
{
    QImage swappedImage;
    unsigned char* imageData;
    unsigned long swappedImageSize;
    std::vector<uint8_t> imageDataIn;
    std::chrono::high_resolution_clock::time_point startTime, stopTime;
    int timeElapsed;
    uint8_t* input;

    startTime = std::chrono::high_resolution_clock::now();

    swappedImage = sentImage.scaled(wantedHeight, wantedWidth, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).rgbSwapped();
    imageData = swappedImage.bits();
    imageDataIn.resize(unsigned(swappedImage.height() * swappedImage.width() * wantedChannels));
    swappedImageSize = unsigned(swappedImage.height() * swappedImage.width() * swappedImage.depth()) / BITS_TO_BYTE;

    for (unsigned long i = 0, j = 0; i < swappedImageSize; i++) {
        if (i%4 == 3)
            continue;
        imageDataIn[j++] = imageData[i];
    }

    input = tfliteInterpreter->typed_input_tensor<uint8_t>(0);

    for (unsigned int i = 0; i < imageDataIn.size(); i++)
        input[i] = uint8_t(imageDataIn.data()[i]);

    tfliteInterpreter->Invoke();

    for (int i = 0; tfliteInterpreter->typed_output_tensor<float>(2)[i] > float(DETECT_THRESHOLD); i++){
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(1)[i]);          //item
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(2)[i]);          //confidence
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4]);      //box ymin
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 1]);  //box xmin
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 2]);  //box ymax
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 3]);  //box xmax
    }

    stopTime = std::chrono::high_resolution_clock::now();
    timeElapsed = int(std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count());

    emit sendOutputTensor(outputTensor, timeElapsed);
    outputTensor.clear();
}

void tfliteWorker::receiveNumOfInferenceThreads(int threads)
{
    numberOfInferenceThreads = threads;
}
