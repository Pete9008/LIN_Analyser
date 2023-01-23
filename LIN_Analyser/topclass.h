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

#ifndef TOPCLASS_H
#define TOPCLASS_H

#include <QObject>

#include "serialclass.h"

class topClass : public QObject
{
    Q_OBJECT
public:
    explicit topClass(QObject *parent = nullptr);

    bool parseMessage(void);

signals:
    void triggerMqttLoop(void);

public slots:
    void run(void);
    void sendLoop(void);
    void dataReceived(QByteArray rxData);
    void displayResult(void);

private:
    SerialClass *m_FTDI;
    int m_state;
    uint8_t m_CommandAddress,m_ScanAddress, m_queryAddress;
    uint8_t m_sent_address;
    uint8_t m_dataVal;
    QByteArray m_rxData, m_parsedMessage, m_refMessage;
    uint8_t m_SendData[8];
    QByteArray m_sentMessage;
    bool m_sent, m_send, m_scan, m_query, m_write, m_grabRef;

};

#endif // TOPCLASS_H
