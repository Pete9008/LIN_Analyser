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

#include "topclass.h"

#include <QTimer>
#include <QCoreApplication>
#include <QDebug>
#include <QThread>


#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <sys/ioctl.h>

#define STATUS_EXV 0x35
#define STATUS_BALL 0x03
#define STATUS_EXP 0x05
#define STATUS_HEAT 0x30
#define STATUS_COMP 0x21

#define STATUS_ID STATUS_COMP

#define COMMAND_EXV 0x34
#define COMMAND_BALL 0x2f
#define COMMAND_EXP 0x2f

#define COMMAND_ID COMMAND_EXP

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}


topClass::topClass(QObject *parent) : QObject(parent)
{
}

void topClass::run(void)
{
    m_FTDI = new SerialClass(this);

    if(m_FTDI->openPort())
    {
        qDebug() << "Connected to I/O module";
    }
    else
    {
        qDebug() << "Could not connect to I/O module";
        QCoreApplication::exit(1);
    }

    m_state = 1;
    m_CommandAddress = COMMAND_ID;
    m_ScanAddress = 0;
    m_queryAddress = 0;
    m_sent_address = 0;
    m_dataVal = 0x01;
    m_sentMessage.clear();
    m_send = false;
    m_sent = false;
    m_scan = false;
    m_write = false;
    m_grabRef = false;
    m_query = false;


    m_SendData[0] = 0x40;
    m_SendData[1] = 0x00;
    m_SendData[2] = 0x00;
    m_SendData[3] = 0x00;
    m_SendData[4] = 0;
    m_SendData[5] = 0;
    m_SendData[6] = 0;
    m_SendData[7] = 0;

    connect(m_FTDI, SIGNAL(dataReceived(QByteArray)), this, SLOT(dataReceived(QByteArray)));



    QTimer *timer = new QTimer(this);
    timer->start(200);

    connect(timer, SIGNAL(timeout()), this, SLOT(sendLoop()));
}


void topClass::dataReceived(QByteArray rxData)
{
    m_rxData = rxData;
}


void topClass::sendLoop(void)
{
    QByteArray writeData;
    int rxChar;


    if(_kbhit())
    {
        rxChar = getchar();
        switch(rxChar)
        {
        case 's':
        case 'S':
            m_scan = true;
            m_ScanAddress = 0;
            break;
        case 'q':
        case 'Q':
            m_query = true;
            m_queryAddress = 0;
            break;
        case 'w':
        case 'W':
            m_write = true;
            m_CommandAddress = 0;
            m_dataVal = 0x40;
            break;
        case 'g':
        case 'G':
            m_grabRef = true;
            break;
        case ' ':
            m_CommandAddress = COMMAND_ID;
            m_send = true;
            break;
        case '1':
            m_SendData[0]--;
            break;
        case '!':
            m_SendData[0]++;
            break;
        case '2':
            m_SendData[1]--;
            break;
        case '"':
            m_SendData[1]++;
            break;
        case '3':
            m_SendData[2]--;
            break;
        case 194:
            m_SendData[2]++;
            break;
        case '4':
            m_SendData[3]--;
            break;
        case '$':
            m_SendData[3]++;
            break;
        case '5':
            m_SendData[4]--;
            break;
        case '%':
            m_SendData[4]++;
            break;
        case '6':
            m_SendData[5]--;
            break;
        case '^':
            m_SendData[5]++;
            break;
        case '7':
            m_SendData[6]--;
            break;
        case '&':
            m_SendData[6]++;
            break;
        case '8':
            m_SendData[7]--;
            break;
        case '*':
            m_SendData[7]++;
            break;
        default:
            break;
        }
    }

    if(m_send)
    {
        m_sent_address = m_CommandAddress;
        //writeData = QByteArrayLiteral("\x40\x40\x40\x40\xff\xff\xff\xff");
        writeData.clear();
        writeData[0] = m_SendData[0];
        writeData[1] = m_SendData[1];
        writeData[2] = m_SendData[2];
        writeData[3] = m_SendData[3];
//        writeData[4] = m_SendData[4];
//        writeData[5] = m_SendData[5];
//        writeData[6] = m_SendData[6];
//        writeData[7] = m_SendData[7];
        m_FTDI->serialWrite(m_CommandAddress, writeData);
        m_sentMessage = "Send  :";
        m_sentMessage.append(writeData.toHex(':'));
        m_sent = true;
        m_send = false;
    }
    else
    {
        if(m_scan)
        {
            //send status request on all IDs
            m_sent_address = m_ScanAddress;
            writeData.clear();
            m_FTDI->serialWrite(m_sent_address, writeData);
            qDebug() << "Sent ID: " << QByteArray::number(m_sent_address,16);
            QTimer::singleShot(20, this, SLOT(displayResult()));
            m_ScanAddress++;
            if(m_ScanAddress >= 60)
            {
                m_scan = false;
            }
        }
        else if(m_write) //write moving ones pattern to all addresses to see if valve moves
        {
            //send test write on all IDs
            m_sent_address = m_CommandAddress;
            writeData.clear();
            for(int i=0;i<4;i++)
            {
                m_SendData[i] = m_dataVal;
                writeData[i] = m_SendData[i];
            }

            qDebug() << "Sent ID: " << QByteArray::number(m_CommandAddress,16);
            m_CommandAddress++;
            if(m_CommandAddress >= 60)
            {
                m_CommandAddress = 0;
                if(m_dataVal < 0x80)
                    m_dataVal = m_dataVal << 1;
                else
                    m_write = false;
            }
            QThread::msleep(50); //wait for command to go out
            m_FTDI->serialClear();
            //send status request on standard ID
            m_sent_address = STATUS_ID;
            writeData.clear();
            m_FTDI->serialWrite(STATUS_ID, writeData);
            QTimer::singleShot(20, this, SLOT(displayResult()));
        }
        else if(m_query) //use to send to all addresses to see if status reg change can be triggered
        {
            //send test write on all IDs
            m_sent_address = m_queryAddress;
            writeData.clear();
            writeData = QByteArrayLiteral("\x55\x55\x55\x55\x55\x55\x55\x55");
            m_FTDI->serialWrite(m_queryAddress, writeData);
            qDebug() << "Sent ID: " << QByteArray::number(m_sent_address,16);
            m_queryAddress++;
            if(m_queryAddress >= 60)
            {
                m_query = false;
            }
            QThread::msleep(20); //wait for command to go out
            m_FTDI->serialClear();
            //send status request on standard ID
            m_sent_address = STATUS_ID;
            writeData.clear();
            m_FTDI->serialWrite(STATUS_ID, writeData);
            QTimer::singleShot(20, this, SLOT(displayResult()));
        }
        else
        {
            //send status request on standard ID
            m_sent_address = STATUS_ID;
            writeData.clear();
            m_FTDI->serialWrite(STATUS_ID, writeData);
            QTimer::singleShot(20, this, SLOT(displayResult()));


        }
    }
}

void topClass::displayResult(void)
{
    uint16_t rx_csum = 0,csum = 0;
    QByteArray line;

    if(m_rxData.size()<=2)
        qDebug() << "No response               ";
    else
    {
        //if((m_rxData.size()==11) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //EXV
        //if((m_rxData.size()==5) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //Ball valve
        if((m_rxData.size()==11) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //Heater
        {
            int i;
            //for(int i=1;i<10;i++) //EXV
            //for(i=2;i<4;i++) //Ball valve
            for(i=1;i<10;i++) //Heater
            {
                csum += (uint8_t)m_rxData[i];
                if(csum >= 256)
                    csum -= 255;
            }
            csum = (~csum) & 0x00ff;
            rx_csum = (uint16_t)(uint8_t)m_rxData[i];
            if(csum == rx_csum)
            {

                if(m_rxData.mid(2,8) != m_refMessage)
                {
                    //qDebug() << "Changed  :" << m_rxData.mid(2,8).toHex();
                    line = "Changed  :";
                    //QCoreApplication::exit(0);
                }
                else
                {
                   // qDebug() << "Received :" << m_rxData.mid(2,8).toHex();
                    line = "Received :";
                }
            }
            else
            {
                //qDebug() <<"CSUM error" << "Rx:" << m_rxData.mid(10,1).toHex() << ", Expect:" << QByteArray::number(csum,16);
                line = "CSUM Err :";
            }
        }
        else
        {
            //qDebug() << m_rxData.toHex();
            line = "Unexpect :";
        }
    }
    line.append(m_rxData.mid(2,8).toHex(':'));
    line.append("    Send Data ");
    for(int i=0;i<8;i++)
    {
        line.append(':');
        line.append(QByteArray::number(m_SendData[i],16));
    }
    //line.append(m_sentMessage);
    if(m_sent)
        line.append("    SENT");
    m_sent = false;
    qDebug() << line;

    if(m_grabRef)
    {
        if(parseMessage())
        {
            m_refMessage = m_parsedMessage;
            qDebug() << "Grab Ref OK";
        }
        else
            qDebug() << "Grab Ref Failed";
        m_grabRef = false;
    }

    m_rxData.clear();
}

bool topClass::parseMessage(void)
{
    uint16_t rx_csum = 0,csum = 0;
    bool result = false;

    m_parsedMessage.clear();

    //if((m_rxData.size()==11) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //EXV
    //if((m_rxData.size()==5) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //Ball valve
    if((m_rxData.size()==11) &&((uint8_t)m_rxData[0] == 0x55) &&((m_rxData[1]&0x3f)== m_sent_address)) //Heater
    {
        int i;
        //for(i=1;i<10;i++) //EXV
        //for(i=2;i<4;i++) //Ball valve
        for(i=1;i<10;i++) //Heater
        {
            csum += (uint8_t)m_rxData[i];
            if(csum >= 256)
                csum -= 255;
        }
        csum = (~csum) & 0x00ff;
        rx_csum = (uint16_t)(uint8_t)m_rxData[i];
        if(csum == rx_csum)
        {
            m_parsedMessage = m_rxData.mid(2,8);
            result = true;
        }
        else
        {
            qDebug() << "Error:" << m_rxData.toHex();
            qDebug() <<"CSUM error" << "Rx:" <<m_rxData.mid(10,1).toHex() << ", Expected:" << QByteArray::number(csum,16);
        }
    }
    else
        qDebug() << "Header Error" << m_rxData.toHex();

    m_rxData.clear();
    return result;
}





