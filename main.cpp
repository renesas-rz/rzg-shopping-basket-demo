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

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption cameraOption(QStringList() << "c" << "camera","Choose a camera.","file");
    QString cameraLocation;
    QString modelLocation;
    QString applicationDescription = \
    "Shopping Basket Demo\n"\
    "  Draws boxes around detected shopping items, displays the name\n"
    "  and confidence of the object, populates a checkout list, and\n"
    "  also displays inference time.\n\n"\
    "Required Hardware:\n"\
    "  Camera: Currently supports ELP 2.0 MP Camera Module, should\n"\
    "          work with any UVC compatible USB camera that has a\n"\
    "          supported resolution of 1280x1024.\n\n"\
    "Buttons:\n"\
    "  Run: Run inference on the selected image once.\n"\
    "  Load Image: Load an image from the filesystem. Supports all\n"
    "  formats supported by QImage.\n"\
    "  Load Webcam: Load a webcam stream.\n"\
    "  Capture Image: Capture an image from the webcam.\n"\
    "  Continuous Checkbox: Only available when a webcam stream is loaded.\n"\
    "                       Enable to continuously run inference.\n"\
    "  Stop: Stop continuous inference.\n"\
    "  Threads: Change the number of inference threads.\n"\
    "  About->License: Read the license that this app is licensed under.\n"\
    "  Camera->Reset: Reset the connection to the webcam.\n"\
    "  Camera->Disconnect: Disconnect the currently connected webcam.\n\n"\
    "Default options:\n"\
    "  Camera: /dev/v4l/by-id/<first file>\n";

    parser.addOption(cameraOption);
    parser.addHelpOption();
    parser.setApplicationDescription(applicationDescription);
    parser.process(a);
    cameraLocation = parser.value(cameraOption);

    if (cameraLocation.isEmpty() && QDir("/dev/v4l/by-id").exists())
        cameraLocation = QDir("/dev/v4l/by-id").entryInfoList(QDir::NoDotAndDotDot).at(0).absoluteFilePath();

    modelLocation = CPU_MODEL_NAME;
    if (!QFile::exists(modelLocation))
            qFatal("%s not found in the current directory", \
                    modelLocation.toStdString().c_str());

    MainWindow w(nullptr, cameraLocation, modelLocation);
    w.show();
    return a.exec();
}
