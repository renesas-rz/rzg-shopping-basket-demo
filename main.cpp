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

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption cameraOption(QStringList() << "c" << "camera", "Choose a camera.", "file");
    QString cameraLocation;
    QString modelLocation;
    QString applicationDescription =
    "Shopping Basket Demo\n"
    "  Draws boxes around detected shopping items, displays the name\n"
    "  and confidence of the object, populates a checkout list, and\n"
    "  also displays inference time.\n\n"
    "Required Hardware:\n"
    "  Camera: Currently Google Coral Mipi Camera is supported,"
    "          but should work with any UVC compatible USB camera.\n\n"
    "Buttons:\n"
    "  Process Basket: Grabs the frame and runs inference.\n"
    "  About->Hardware: Display the board information.\n"
    "  About->License: Read the license that this app is licensed under.\n"
    "  About->Exit: Close the application.\n"
    "Default options:\n"
    "  Camera: /dev/video0\n";

    parser.addOption(cameraOption);
    parser.addHelpOption();
    parser.setApplicationDescription(applicationDescription);
    parser.process(a);
    cameraLocation = parser.value(cameraOption);

    modelLocation = CPU_MODEL_NAME;

    if (!QFile::exists(modelLocation))
            qFatal("%s not found in the current directory",
                    modelLocation.toStdString().c_str());

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    MainWindow w(nullptr, cameraLocation, modelLocation);
    w.show();
    return a.exec();
}
