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

#ifndef VIDEOWORKER_H
#define VIDEOWORKER_H

#include <QObject>

class videoWorker : public QObject
{
    Q_OBJECT

public:
    explicit videoWorker(QObject *parent = 0);
    void setDelayMS(unsigned int delay);

signals:
    void showVideo();

public slots:
    void StartVideo();
    void StopVideo();

private slots:
    void play_video();

private:
    bool stopped;
    bool running;
    unsigned int videoDelay;
};

#endif // VIDEOWORKER_H
