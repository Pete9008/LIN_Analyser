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

#include "serialclass.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QCoreApplication>
#include <QThread>

SerialClass::SerialClass(QObject *parent) : QObject(parent)
{
    m_serialPort = new QSerialPort(this);

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialClass::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialClass::handleError);
}

bool SerialClass::openPort(void)
{
    QString description;
    QString manufacturer;
    QString serialNumber;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        if((!info.manufacturer().isEmpty() && info.manufacturer().contains("FTDI",Qt::CaseSensitive)) &&
           (!info.description().isEmpty() && info.description().contains("TTL232R-3V3",Qt::CaseSensitive)))
        {
            m_serialPort->setPortName(info.portName());
            //m_serialPort->setBaudRate(QSerialPort::Baud9600);
            m_serialPort->setBaudRate(QSerialPort::Baud19200);
            m_serialPort->setDataBits(QSerialPort::Data8);
            m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
            m_serialPort->setStopBits(QSerialPort::OneStop);
            if (m_serialPort->open(QIODevice::ReadWrite))
                return true;
        }
    }
    return false;
}

#define BIT(data,shift) ((data>>shift)&0x01)
bool SerialClass::serialWrite(uint8_t address, QByteArray writeData)
{
    uint16_t csum = 0;

    m_serialPort->setBreakEnabled(true);
    //QThread::usleep(1400); //13bits at 9600baud
    QThread::usleep(700); //13bits at 19200baud
    m_serialPort->setBreakEnabled(false);
    QByteArray message = QByteArrayLiteral("\x55");

    uint8_t p0 = BIT(address,0) ^ BIT(address,1) ^ BIT(address,2) ^ BIT(address,4);
    uint8_t p1 = (~(BIT(address,1) ^ BIT(address,3) ^ BIT(address,4) ^ BIT(address,5)))&0x01;
    uint8_t headerByte = address | (p0<<6) | (p1<<7);
    message.append(headerByte);
    m_serialPort->clear(QSerialPort::AllDirections);


    if(!writeData.isEmpty())
    {
        message.append(writeData);

        //csum = headerByte; //EXV
        csum = 0; //Ball valve
        for(int i=0;i<writeData.size();i++)
        {
            csum += (uint8_t)writeData[i];
            if(csum >= 256)
                csum -= 255;
        }
        csum = (~csum) & 0x00ff;

        message.append((uint8_t)csum);
    }

    qint64 bytesWritten = m_serialPort->write(message);
    m_serialPort->flush();

    if (bytesWritten == message.size())
        return true;
    else
        return false;

}

void SerialClass::handleReadyRead()
{
    QByteArray data;

    data.append(m_serialPort->readAll());

    emit dataReceived(data);
}

void SerialClass::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        QCoreApplication::exit(1);
    }
    else if (serialPortError == QSerialPort::WriteError)
    {
        QCoreApplication::exit(1);
    }
}

void SerialClass::serialClear(void)
{
    m_serialPort->clear(QSerialPort::AllDirections);
}




