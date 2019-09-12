#include "client.h"
#include <QHostAddress>
#include <QDateTime>
#include <QDir>
#include <QDataStream>
#include <QHostInfo>

Client::Client(qintptr descriptor, QObject *parent)
	: QObject(parent)
	, m_tunnel(false)
	, m_auth(false)
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

	if( m_auth ) app::changeUserConnection( m_user.login, -1 );

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

	app::setLog(5,QString("Client::slot_clientReadyRead %1 bytes [%2] [%3]").arg(buff.size()).arg(QString(buff)).arg(QString(buff.toHex())));

	// Если данные уже отправили выходим
	if( buff.size() == 0 ) return;

	//Client::Pkt* pkt = reinterpret_cast<Client::Pkt*>(buff.data());
	Client::Pkt pkt;
	QDataStream stream(&buff, QIODevice::ReadWrite);
	stream.setByteOrder(QDataStream::BigEndian);
	stream >> pkt.version;

	// Приветствие
	switch( pkt.version ){
		// Добавление протокола конфигурации и отладки
		case Client::Proto::Version::CONFIGURE:
			// Приняли данные, распарсили и закрыли соединение!!! (Только для локалхоста)
			if( m_pClient->peerAddress().toString() == "127.0.0.1" ){
				stream >> pkt.cmd;
				stream >> pkt.len;
				pkt.rawGet = buff.mid( 3, pkt.len );
				parsAdminPkt( pkt.cmd, pkt.rawRet, pkt.rawGet );
				sendToClient( pkt.rawRet );
			}
			slot_stop();
		break;
		case Client::Proto::Version::SOCKS4:
			stream >> pkt.cmd;
			stream >> pkt.port;
			stream >> pkt.ip;
			pkt.addr.setAddress( pkt.ip );
			app::setLog(5,QString("Client::slot_clientReadyRead [%1,%2,%3,%4] from [%5]").arg(pkt.version).arg(pkt.cmd).arg(pkt.port).arg(QHostAddress(pkt.ip).toString()).arg(m_pClient->peerAddress().toString()));
			if( !app::isSocks4Access( m_pClient->peerAddress() ) ){
				sendError( pkt.version, "is NOT ACCESS SOCKS4", 0, 2 );
				return;
			}
		break;
		case Client::Proto::Version::AUTH_LP:
			if( !parsAuthPkt( buff ) ){
				sendError( Client::Proto::Version::SOCKS5, "Packet is not correct" );
				return;
			}

			if( app::getUserConnectionsNum( m_user.login ) < m_user.maxConnections ){
				app::changeUserConnection( m_user.login, 1 );
			}else{
				m_auth = false;
				app::setLog(4,QString("Client::parsAuthPkt() limit connections [%1]").arg(QString( m_user.login )));
			}

			pkt.rawRet.clear();
			pkt.rawRet[0] = Client::Proto::Version::AUTH_LP;
			pkt.rawRet[1] = ( m_auth ) ? 0x00 : 0x01;
			sendToClient( pkt.rawRet );
			return;
		break;
		case Client::Proto::Version::SOCKS5:
			// is BAN ?
			if( app::isBan( m_pClient->peerAddress() ) ){
				sendError( pkt.version, QString("IP is BAN"), 0x05, 2 );
				return;
			}

			pkt.rawRet.clear();
			pkt.rawRet[0] = Client::Proto::Version::SOCKS5;

			if( !m_auth ){
				if( !parsAuthMethods( buff ) ){
					pkt.rawRet[1] = 0xFF;
				}else{
					pkt.rawRet[1] = 0x02;
				}
				sendToClient( pkt.rawRet );
				return;
			}else{
				if( !parsConnectPkt( buff, pkt.addr, pkt.port, pkt.domainAddr ) ){
					sendError( pkt.version, QString("Pkt is not correct") );
					return;
				}
			}
		break;
		default: return;
	}


	app::setLog(4,QString("Client::slot_clientReadyRead request connect [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(QHostAddress(pkt.addr).toString()).arg(pkt.port));

	if( m_auth ){
		bool isBlockIP = app::isBlockedUserList( m_user.login, pkt.addr.toString() );
		bool isAccessIP = app::isAccessUserList( m_user.login, pkt.addr.toString() );
		bool isBlockAddr = app::isBlockedUserList( m_user.login, pkt.domainAddr );
		bool isAccessAddr = app::isAccessUserList( m_user.login, pkt.domainAddr );
		if( (isBlockIP && !isAccessIP && !isAccessAddr) ){
			app::setLog(4,QString("Client::slot_clientReadyRead findUserBlockIP [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(pkt.addr.toString()).arg(pkt.port));
			sendError( pkt.version, "address is blocked" );
			return;
		}
		if( (isBlockAddr && !isAccessIP && !isAccessAddr) ){
			app::setLog(4,QString("Client::slot_clientReadyRead findUserBlockAddr [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(pkt.domainAddr).arg(pkt.port));
			sendError( pkt.version, "address is blocked" );
			return;
		}
	}

	if( app::isBlockAddr( pkt.addr ) ){
		app::setLog(4,QString("Client::slot_clientReadyRead findBlockAddr [%1]->[%2:%3]").arg(m_pClient->peerAddress().toString()).arg(QHostAddress(pkt.ip).toString()).arg(pkt.port));
		sendError( pkt.version, "address is blocked" );
		return;
	}

	m_pTarget->connectToHost( pkt.addr, pkt.port);
	m_pTarget->waitForConnected(3000);
	if( m_pTarget->isOpen() ){
		pkt.rawRet.clear();
		switch( pkt.version ){
			case Client::Proto::Version::SOCKS4:
				pkt.rawRet[0] = 0x00;
				pkt.rawRet[1] = 0x5a;
				pkt.rawRet[2] = 0x00;
				pkt.rawRet[3] = 0x00;
				pkt.rawRet[4] = 0x00;
				pkt.rawRet[5] = 0x00;
				pkt.rawRet[6] = 0x00;
				pkt.rawRet[7] = 0x00;
			break;
			case Client::Proto::Version::SOCKS5:
				pkt.rawRet = buff;
				pkt.rawRet[0] = Client::Proto::Version::SOCKS5;
				pkt.rawRet[1] = 0x00;
			break;
		}
		sendToClient( pkt.rawRet );
		m_tunnel = true;
		return;
	}else{
		sendError( pkt.version, "is not connected" );
		return;
	}

	if(buff.size() > 0) app::setLog(3,QString("buff.size() > 0 !!!"));
}

void Client::slot_targetReadyRead()
{
	if( !m_pTarget->isReadable()) m_pTarget->waitForReadyRead(300);
	while( m_pTarget->bytesAvailable() ) sendToClient( m_pTarget->read( 1024 ) );
}

void Client::sendError(const uint8_t protoByte, const QString &errorString, const uint8_t errorCode, const uint8_t level)
{
	app::setLog( level, QString("Client::sendError: %1 from [%2]").arg(errorString).arg(m_pClient->peerAddress().toString()) );

	QByteArray ret;
	ret.append( protoByte );

	switch(protoByte){
		case Client::Proto::Version::SOCKS4:
			ret.resize(8);
			ret[0] = 0x00;
			ret[1] = 0x5b;
			ret[2] = 0x00;
			ret[3] = 0x00;
			ret[4] = 0x00;
			ret[5] = 0x00;
			ret[6] = 0x00;
			ret[7] = 0x00;
		break;
		case Client::Proto::Version::SOCKS5:
			ret.append( errorCode );
			ret.append( '\0' );
			ret.append( '\0' );
			ret.append( '\0' );
			ret.append( '\0' );
			ret.append( '\0' );
			ret.append( '\0' );
			ret.append( '\0' );
		break;
		default: break;
	}

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
	app::setLog(5,QString("Client::sendToClient %1 bytes [%2]").arg(data.size()).arg(QString(data.toHex())));
}

void Client::sendToTarget(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pTarget->state() == QAbstractSocket::ConnectingState ) m_pTarget->waitForConnected(300);
	if( m_pTarget->state() == QAbstractSocket::UnconnectedState ) return;
	m_pTarget->write( data );
	m_pTarget->waitForBytesWritten(100);
}

bool Client::parsAuthPkt(QByteArray &data)
{
	bool res = false;
	uint8_t loginLen;
	uint8_t passLen;
	QByteArray login;
	QByteArray pass;

	//пакет не полный
	if( data.size() < 2 ) return res;
	loginLen = data[1];
	if( data.size() < (loginLen + 2) ) return res;
	login = data.mid( 2, loginLen );
	data.remove( 0, (loginLen + 2) );
	if( data.size() < 1 ) return res;
	passLen = data[0];
	if( data.size() < (passLen + 1) ) return res;
	pass = data.mid( 1, loginLen );
	res = app::chkAuth( login, pass );

	if( res ){
		m_user = app::getUserData( login );
		m_auth = true;
		app::setLog(4,QString("Client::parsAuthPkt() auth success [%1]").arg(QString(login)));
	}else{
		app::addBAN( m_pClient->peerAddress() );
		app::setLog(3,QString("Client::parsAuthPkt() auth error [%1:%2]").arg(QString(login)).arg(QString(pass)));
	}

	return res;
}

bool Client::parsAuthMethods(QByteArray &data)
{
	bool res = false;

	//пакет не полный
	if( data.size() < 2 ) return res;
	uint8_t numAuthMethods = data[1];
	//пакет не корректный
	if( numAuthMethods < 1 ) return res;
	if( data.size() < (numAuthMethods + 2) ) return res;
	// поиск метода авторизации 0x02 (login/password)
	for( uint8_t i = 0; i < numAuthMethods; i++ ){
		if( data.at(i+2) == 0x02 ){
			res = true;
			break;
		}
	}

	return res;
}

bool Client::parsConnectPkt(QByteArray &data, QHostAddress &addr, uint16_t &port, QString &domainAddr)
{
	bool res = false;

	//пакет не полный
	if( data.size() < 5 ) return res;

	uint8_t typeAddr = data[3];
	QByteArray ip;
	QByteArray portBA;
	uint8_t nameLen = 0;
	QHostInfo info;

	switch( typeAddr ){
		case 0x01:	//IPv4
			//пакет не полный
			if( data.size() < 10 ) return res;
			ip = data.mid( 4, 4 );
			portBA = data.mid( 8, 2 );
			parsIP( ip, addr );
			parsPORT( portBA, port );
			res = true;
		break;
		case 0x03:	//domain name
			nameLen = data[4];
			if( data.size() < (7 + nameLen) ) return res;
			ip = data.mid( 5, nameLen );
			domainAddr = QString(ip);
			info = QHostInfo::fromName( domainAddr );
			if( info.error() == QHostInfo::NoError && !app::isBlockedDomName( domainAddr ) ){
				for(auto elem:info.addresses()){
					if( !app::isBlockAddr( elem ) ){
						addr = elem;
						break;
					}
				}
			}else{
				return res;
			}
			portBA = data.mid( (5 + nameLen), 2 );
			parsPORT( portBA, port );
			res = true;
		break;
		case 0x04:	//IPv6
			//пакет не полный
			if( data.size() < 22 ) return res;
			ip = data.mid( 4, 16 );
			portBA = data.mid( 20, 2 );
			parsIP( ip, addr );
			parsPORT( portBA, port );
			res = true;
		break;
	}

	return res;

}

void Client::parsIP(QByteArray &data, QHostAddress &addr)
{
	if( data.size() == 16 ) addr.setAddress( (uint8_t*)data.data() );

	uint32_t ip;

	QDataStream stream(&data, QIODevice::ReadWrite);
	stream.setByteOrder(QDataStream::BigEndian);
	stream >> ip;

	addr.setAddress( ip );
}

void Client::parsPORT(QByteArray &data, uint16_t &port)
{
	QDataStream stream(&data, QIODevice::ReadWrite);
	stream.setByteOrder(QDataStream::BigEndian);
	stream >> port;
}

void Client::parsAdminPkt(const uint8_t cmd, QByteArray &sendData, const QByteArray &data)
{
	sendData.clear();

	QString str;
	QStringList strList;

	switch(cmd){
		case 0xFF:		// Get version
			sendData.append( app::conf.version );
		break;
		case 0x10:		// Get Users Info
			for( auto user:app::conf.users ){
				str = QString("%1	%2	%3	%4").arg( user.login ).arg( app::getUserConnectionsNum( user.login ) ).arg( user.maxConnections ).arg( user.lastLoginTimestamp );
				strList.push_back( str );
			}
			sendData.append( strList.join(";") );
		break;
		case 0x11:		// reload settings
			app::loadSettings();
		break;
		case 0x12:		// get black domains
			for( auto addr:app::accessList.blackDomains ) strList.push_back( addr );
			sendData.append( strList.join(";") );
		break;
		case 0x13:		// add black domains
			for( auto addr:data.split(';') ) app::addGlobalBlackAddr( QString(addr) );
		break;
		default: break;
	}
}
