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

#include <chrono>

#include "tfliteworker.h"

#include <opencv2/imgproc/imgproc.hpp>

#ifndef SBD_X86
#include <armnn/ArmNN.hpp>
#include <armnn/Utils.hpp>
#include <delegate/armnn_delegate.hpp>
#include <delegate/DelegateOptions.hpp>
#endif

tfliteWorker::tfliteWorker(QString modelLocation, bool armnnDelegate, int defaultThreads)
{
    tflite::ops::builtin::BuiltinOpResolver tfliteResolver;
    TfLiteIntArray *wantedDimensions;

    numberOfInferenceThreads = defaultThreads;

    tfliteModel = tflite::FlatBufferModel::BuildFromFile(modelLocation.toStdString().c_str());
    tflite::InterpreterBuilder(*tfliteModel, tfliteResolver) (&tfliteInterpreter);

#ifndef SBD_X86
    /* Setup the delegate */
    if(armnnDelegate == true) {
        std::vector<armnn::BackendId> backends = {armnn::Compute::CpuAcc};
        armnnDelegate::DelegateOptions delegateOptions(backends);
        std::unique_ptr<TfLiteDelegate, decltype(&armnnDelegate::TfLiteArmnnDelegateDelete)>
            armnnTfLiteDelegate(armnnDelegate::TfLiteArmnnDelegateCreate(delegateOptions),
            armnnDelegate::TfLiteArmnnDelegateDelete);

        /* Instruct the Interpreter to use the armnnDelegate */
        if (tfliteInterpreter->ModifyGraphWithDelegate(std::move(armnnTfLiteDelegate)) != kTfLiteOk)
           qWarning("Delegate could not be used to modify the graph\n");
    }
#endif

    if (tfliteInterpreter->AllocateTensors() != kTfLiteOk)
        qFatal("Failed to allocate tensors!");

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
void tfliteWorker::receiveImage(const cv::Mat& sentMat)
{
    cv::Mat sentImageMat;
    std::chrono::high_resolution_clock::time_point startTime, stopTime;
    int timeElapsed;
    int input;

    if(sentMat.empty()) {
        qWarning("Received invalid image path, cannot run inference");
        return;
    }

    input = tfliteInterpreter->inputs()[0];

    cv::resize(sentMat, sentImageMat, cv::Size(wantedHeight, wantedWidth));

    memcpy(tfliteInterpreter->typed_tensor<uint8_t>(input), sentImageMat.data, sentImageMat.total() * sentImageMat.elemSize());

    startTime = std::chrono::high_resolution_clock::now();
    tfliteInterpreter->Invoke();
    stopTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; tfliteInterpreter->typed_output_tensor<float>(2)[i] > float(DETECT_THRESHOLD)
         && tfliteInterpreter->typed_output_tensor<float>(2)[i] <= float(1.0); i++) {
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(1)[i]);          //item
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(2)[i]);          //confidence
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4]);      //box ymin
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 1]);  //box xmin
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 2]);  //box ymax
        outputTensor.push_back(tfliteInterpreter->typed_output_tensor<float>(0)[i * 4 + 3]);  //box xmax
    }

    timeElapsed = int(std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count());
    emit sendOutputTensor(outputTensor, timeElapsed, sentMat);
    outputTensor.clear();
}

void tfliteWorker::receiveNumOfInferenceThreads(int threads)
{
    numberOfInferenceThreads = threads;
}
