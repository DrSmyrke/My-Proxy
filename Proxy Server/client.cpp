#include "client.h"
//TODO: remove qDebug
#include <QDebug>
#include <QHostAddress>

Client::Client(QObject *parent) : QObject(parent)
{
	m_pClient = new QTcpSocket(this);
	m_pTarget = new QTcpSocket(this);



	connect( m_pClient, &QTcpSocket::readyRead, this, &Client::slot_clientReadyRead );
}

void Client::setSocketDescriptor(qintptr socketDescriptor)
{
	m_pClient->setSocketDescriptor( socketDescriptor );
	qDebug()<<m_pClient->peerAddress().toString();
}

void Client::stop()
{
	m_pClient->close();
	emit signal_finished( m_pClient->socketDescriptor() );
}

void Client::slot_clientReadyRead()
{
	QByteArray buff;
	while( m_pClient->bytesAvailable() ) buff.append( m_pClient->readAll() );

	qDebug()<<buff.toHex();
}
/*
 * "192.168.1.149"
"04010050ac1d4e0200"
 *
 *
 * "127.0.0.1"
"04010050ac1d4e0200"
 *
 *
 *
 *
 * */
