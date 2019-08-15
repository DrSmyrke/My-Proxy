#include "client.h"
#include <QHostAddress>
#include <QDateTime>
#include <QDir>
#include <QDataStream>

Client::Client(qintptr descriptor, QObject *parent)
	: QObject(parent)
	, m_tunnel(false)
{
	m_pClient = new QTcpSocket();
	m_pTarget = new QTcpSocket();

	if( !m_pClient->setSocketDescriptor( descriptor ) ) slot_stop();

	connect( m_pTarget, &QTcpSocket::readyRead, this, &Client::slot_targetReadyRead);
	connect( m_pClient, &QTcpSocket::readyRead, this, &Client::slot_clientReadyRead);

	connect( m_pClient, &QTcpSocket::disconnected, this, [this](){
		app::setLog(5,QString("Client::Client disconnected"));
		slot_stop();
	});
	connect( m_pTarget, &QTcpSocket::disconnected, this, [this](){
		app::setLog(5,QString("Client::Target disconnected"));
		slot_stop();
	});
}

void Client::slot_start()
{
	//m_pRClient->connectToHost(m_dist_addr,m_dist_port);
	//m_pRClient->waitForConnected(3000);
}

void Client::slot_stop()
{
	if( m_pTarget->isOpen() ) m_pTarget->close();
	if( m_pClient->isOpen() ) m_pClient->close();
	emit signal_finished();
}

void Client::slot_clientReadyRead()
{
	QByteArray buff;

	if(!m_pClient->isReadable()) m_pClient->waitForReadyRead(300);

	while( m_pClient->bytesAvailable() ){
		buff.append( m_pClient->read(1024) );
		if( m_pTarget->isOpen() && m_tunnel ){
			sendToTarget( buff );
			buff.clear();
		}
	}

	app::setLog(5,QString("Client::slot_clientReadyRead %1 bytes [%2]").arg(buff.size()).arg(QString(buff)));

	// Если данные уже отправили выходим
	if( buff.size() == 0 ) return;

	//Client::Pkt* pkt = reinterpret_cast<Client::Pkt*>(buff.data());
	Client::Pkt pkt;
	QDataStream stream(&buff, QIODevice::ReadWrite);
	stream.setByteOrder(QDataStream::BigEndian);
	stream >> pkt.version;
	stream >> pkt.cmd;
	stream >> pkt.port;
	stream >> pkt.ip;
	pkt.addr.setAddress( pkt.ip );

	app::setLog(5,QString("Client::slot_clientReadyRead [%1,%2,%3,%4] from [%5]").arg(pkt.version).arg(pkt.cmd).arg(pkt.port).arg(QHostAddress(pkt.ip).toString()).arg(m_pClient->peerAddress().toString()));
	app::setLog(4,QString("Client::slot_clientReadyRead request connect [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(QHostAddress(pkt.ip).toString()).arg(pkt.port));

	if( app::findBlockAddr( pkt.addr ) ){
		app::setLog(4,QString("Client::slot_clientReadyRead findBlockAddr [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(QHostAddress(pkt.ip).toString()).arg(pkt.port));
		sendError();
		slot_stop();
		return;
	}

	if( pkt.addr.toString() == "0.0.0.0" && pkt.port == 10000 ){
		app::loadSettings();
		sendError();
		slot_stop();
		return;
	}
	if( pkt.addr.toString() == "0.0.0.0" && pkt.port == 20000 ){
		app::saveSettings();
		sendError();
		slot_stop();
		return;
	}

	m_pTarget->connectToHost( pkt.addr, pkt.port);
	m_pTarget->waitForConnected(3000);
	if( m_pTarget->isOpen() ){
		QByteArray ret;
		ret[0]=0x00;
		ret[1]=0x5a;
		ret[2]=0x00;
		ret[3]=0x00;
		ret[4]=0x00;
		ret[5]=0x00;
		ret[6]=0x00;
		ret[7]=0x00;
		m_pClient->write(ret,ret.size());
		m_tunnel = true;
		return;
	}else{
		app::setLog(4,QString("Client::slot_clientReadyRead is not connected"));
		sendError();
		slot_stop();
		return;
	}
}

void Client::slot_targetReadyRead()
{
	if( !m_pTarget->isReadable()) m_pTarget->waitForReadyRead(300);
	while( m_pTarget->bytesAvailable() ) sendToClient( m_pTarget->read( 1024 ) );
}

void Client::sendError()
{
	app::setLog( 4, QString("Client::sendError: [%1]").arg(m_pClient->peerAddress().toString()) );

	QByteArray ret;
		ret[0]=0x00;
		ret[1]=0x5b;
		ret[2]=0x00;
		ret[3]=0x00;
		ret[4]=0x00;
		ret[5]=0x00;
		ret[6]=0x00;
		ret[7]=0x00;
	m_pClient->write(ret,ret.size());

	slot_stop();
}

void Client::sendToClient(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pClient->state() == QAbstractSocket::ConnectingState ) m_pClient->waitForConnected(300);
	if( m_pClient->state() == QAbstractSocket::UnconnectedState ) return;
	m_pClient->write(data);
	m_pClient->waitForBytesWritten(100);
}

void Client::sendToTarget(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pTarget->state() == QAbstractSocket::ConnectingState ) m_pTarget->waitForConnected(300);
	if( m_pTarget->state() == QAbstractSocket::UnconnectedState ) return;
	m_pTarget->write( data );
	m_pTarget->waitForBytesWritten(100);
}
