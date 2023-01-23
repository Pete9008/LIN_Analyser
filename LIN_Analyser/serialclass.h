/*
 * This file is part of the LIN_Analyser project
 *
 * Copyright (C) 2022 Pete9008 <openinverter.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERIALCLASS_H
#define SERIALCLASS_H

#include <QObject>

#include <QtSerialPort/QSerialPort>

class SerialClass : public QObject
{
    Q_OBJECT
public:
    explicit SerialClass(QObject *parent = nullptr);

signals:

public slots:
    void handleError(QSerialPort::SerialPortError error);
    void handleReadyRead();

signals:
    void dataReceived(QByteArray data);

public:
    bool openPort(void);
    bool serialWrite(uint8_t address, QByteArray writeData);
    void serialClear(void);

private:
    QSerialPort *m_serialPort;

};

#endif // SERIALCLASS_H
