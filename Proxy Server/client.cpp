#include "client.h"
#include <QHostAddress>
#include <QDateTime>
#include <QDir>
#include <QDataStream>
#include <QHostInfo>
#include <QUrl>

Client::Client(qintptr descriptor, QObject *parent)
	: QObject(parent)
	, m_tunnel(false)
	, m_auth(false)
	, m_proto(Client::Proto::UNKNOWN)
	, m_failedAuthorization(0)
{
	m_pClient = new QTcpSocket();
	m_pTarget = new QTcpSocket();

	if( !m_pClient->setSocketDescriptor( descriptor ) ) slot_stop();

	connect( m_pTarget, &QTcpSocket::readyRead, this, &Client::slot_targetReadyRead);
	connect( m_pClient, &QTcpSocket::readyRead, this, &Client::slot_clientReadyRead);

	connect( m_pClient, &QTcpSocket::disconnected, this, [this](){
		app::setLog(6,QString("Client::Client disconnected"));
		slot_stop();
	});
	connect( m_pTarget, &QTcpSocket::disconnected, this, [this](){
		app::setLog(6,QString("Client::Target disconnected"));
		slot_stop();
	});
}

void Client::run()
{
	app::setLog( 4, QString("Client::Client connected [%1:%2]").arg( m_pClient->peerAddress().toString() ).arg( m_pClient->peerPort() ) );
}

void Client::slot_stop()
{
	if( m_pTarget->isOpen() ) m_pTarget->close();
	if( m_pClient->isOpen() ) m_pClient->close();

	if( m_auth ) app::removeUserConnection( m_userLogin, m_targetHostStr );

	emit signal_finished();
}

void Client::slot_clientReadyRead()
{
	QByteArray buff;

	while( m_pClient->bytesAvailable() ){
		buff.append( m_pClient->read(1024) );
		if( m_pTarget->isOpen() && m_tunnel ){
			sendToTarget( buff );
			//TODO: Переделать лог
			app::setLog(5,QString("ProxyClient::slot_targetReadyRead [%1:%2]").arg(m_pTarget->peerAddress().toString()).arg(m_pTarget->peerPort()));
			if( m_auth ) app::addBytesOutTraffic( m_pTarget->peerAddress().toString(), m_userLogin, buff.size() );
			buff.clear();
		}
	}

	// Если данные уже отправили выходим
	if( buff.size() == 0 ) return;

	app::setLog(5,QString("ProxyClient::slot_clientReadyRead %1 bytes [%2]").arg(buff.size()).arg(QString(buff)));
	app::setLog(6,QString("ProxyClient::slot_clientReadyRead [%3]").arg(QString(buff.toHex())));

	auto pkt = http::parsPkt( buff );

	if( pkt.head.valid && pkt.isRequest ){
		parsHttpProxy( pkt, buff.size() );
	}else{
		sendResponse( 400, "Bad Request" );
		return;
	}

	//if(buff.size() > 0) app::setLog(3,QString("buff.size() > 0 !!! [%1]").arg( QString( buff ) ));
}

void Client::slot_targetReadyRead()
{
	while( m_pTarget->bytesAvailable() ){
		QByteArray buff;
		buff.append( m_pTarget->read( 1024 ) );
		sendToClient( buff );
		if( m_auth ) app::addBytesInTraffic( m_pTarget->peerAddress().toString(), m_userLogin, buff.size() );
	}
}

void Client::sendResponse(const uint16_t code, const QString &comment)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	pkt.body.rawData.append( app::getHtmlPage("Service page",comment.toLatin1()) );
	app::setLog( 5, QString("ProxyClient::sendResponse [%1]").arg(pkt.head.response.code) );
	sendToClient( http::buildPkt(pkt) );
}

void Client::sendRawResponse(const uint16_t code, const QString &comment, const QString &data, const QString &mimeType)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	if( !mimeType.isEmpty() ) pkt.head.contType = mimeType;
	pkt.body.rawData.append( data );
	app::setLog( 5, QString("ProxyClient::sendRawResponse [%1]" ).arg(pkt.head.response.code));
	sendToClient( http::buildPkt(pkt) );
}

void Client::sendToClient(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pClient->state() == QAbstractSocket::ConnectingState ) m_pClient->waitForConnected(300);
	if( m_pClient->state() == QAbstractSocket::UnconnectedState ) return;
	m_pClient->write(data);
	m_pClient->waitForBytesWritten(100);
	app::setLog(5,QString("ProxyClient::sendToClient %1 bytes [%2]").arg(data.size()).arg(QString(data)));
	app::setLog(6,QString("ProxyClient::sendToClient [%3]").arg(QString(data.toHex())));
}

void Client::sendToTarget(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pTarget->state() == QAbstractSocket::ConnectingState ) m_pTarget->waitForConnected(300);
	if( m_pTarget->state() == QAbstractSocket::UnconnectedState ) return;
	if( m_proto == Client::Proto::HTTPS ){
		m_pTarget->write( data );
	}
	if( m_proto == Client::Proto::HTTP ){
		//TODO: Реализовать подмену заголовка и анонимный режим (пересборка пакета)
		m_pTarget->write( data );
	}
	m_pTarget->waitForBytesWritten(100);

	app::setLog(5,QString("ProxyClient::sendToTarget %1 bytes [%2]").arg(data.size()).arg(QString(data)));
	app::setLog(6,QString("ProxyClient::sendToTarget [%2]").arg(QString(data.toHex())));
}

void Client::parsHttpProxy(http::pkt &pkt, const int32_t sizeInData)
{
	if( !pkt.isRequest ) return;

	app::setLog( 4, QString("ProxyClient::parsHttpProxy %1 %2").arg( pkt.head.request.method ).arg( pkt.head.request.target ));

	if( pkt.head.request.method == "CONNECT" ) m_proto = Client::Proto::HTTPS;
	if( pkt.head.request.method == "GET" || pkt.head.request.method == "POST" ) m_proto = Client::Proto::HTTP;

	if( m_proto == http::Proto::UNKNOWN ){
		sendResponse( 400, "Bad Request" );
		slot_stop();
		return;
	}

	// Обработка блокировки и отключения клиента
	if( app::isBan( m_pTarget->peerAddress() ) ){
		app::setLog( 2, QString("ProxyClient::parsHttpProxy client is isBan %1 %2").arg( m_userLogin ).arg( m_pClient->peerAddress().toString() ));
		sendNoAccess();
		slot_stop();
		return;
	}

	if( !pkt.head.proxyAuthorization.isEmpty() ) parsAuth( pkt.head.proxyAuthorization, pkt.head.request.method );
	if( !m_auth ){
		if( m_failedAuthorization >= app::conf.maxFailedAuthorization ){
			sendResponse( 423, "Locked" );
			slot_stop();
			return;
		}
		m_failedAuthorization++;
		sendNoAuth();
		return;
	}

	QUrl url;

	switch ( m_proto ) {
		case Client::Proto::HTTPS:
			m_targetHostStr = pkt.head.request.target;
			app::setLog(4,QString("ProxyClient::parsHttpProxy HTTPS Request to [%1]").arg( m_targetHostStr ));
			m_targetHostPort = 443;
		break;
		case Client::Proto::HTTP:
			url.setUrl( pkt.head.request.target );
			if( !url.isValid() ){
				sendResponse( 400, "Bad Request" );
				return;
			}else{
				m_targetHostStr = QString("%1:%2").arg( url.host() ).arg( url.port(80) );
				pkt.head.request.target = url.path();
				if( !url.query().isEmpty() ) pkt.head.request.target += "?" + url.query();
				pkt.head.request.target.replace( " ", "%20" );
			}

			app::setLog(4,QString("ProxyClient::parsHttpProxy HTTP Request to [%1] %2").arg( m_targetHostStr ).arg( pkt.head.request.target ));
			pkt.head.proxyAuthorization.clear();
			pkt.head.connection = "close";
			m_targetHostPort = 80;
		break;
	}

	auto tmp = m_targetHostStr.split( ":" );
	if( tmp.size() == 2 ){
		if( !tmp[1].isEmpty() ) m_targetHostPort = tmp[1].toUShort();
		m_targetHostStr = tmp[0];
	}

	if( m_targetHostStr.isEmpty() ){
		app::setLog(4,QString("ProxyClient::parsHttpProxy Bad Request [%1]").arg( m_targetHostStr ));
		sendResponse( 400, "Bad Request" );
		return;
	}

	if( app::isBlockedDomName( m_targetHostStr ) ){
		app::setLog(4,QString("ProxyClient::parsHttpProxy isBlockedDomName [%1]").arg( m_targetHostStr ));
		sendResponse( 423, "Locked" );
		slot_stop();
		return;
	}

	std::vector<Host> targets;

	app::getIPsFromDomName( m_targetHostStr, m_targetHostPort, targets );

	if( m_auth ){
		//Если превысили лимит на траффик
		if( app::isTrafficLimit( m_userLogin ) ){
			app::setLog(4,QString("ProxyClient::parsHttpProxy isTrafficLimit [%1]").arg( m_targetHostStr ));
			targets.clear();
			Host host;
			host.ip.setAddress( "127.0.0.1" );
			host.port = app::conf.controlPort;
			targets.push_back( host );
		}
	}

	// Превышение числа коннектов на пользователя
	if( app::isMaxConnections( m_userLogin ) ){
		bool find = false;
		if( targets.size() == 1 ){
			// Игнорируем ограничение по количеству подключения для входа в вебморду
			if( targets[0].ip.toString() == "127.0.0.1" && targets[0].port == app::conf.controlPort && m_auth ) find = true;
		}
		if( !find ){
			app::setLog(4,QString("ProxyClient::parsHttpProxy Too Many Requests [%1]").arg( m_userLogin ));
			sendResponse(429,"Too Many Requests");
			slot_stop();
			return;
		}
	}

	// Наконец то пробуем законнектиться

	for( auto host:targets ){
		if( app::isBlockHost( host ) ){
			app::setLog( 4, QString("ProxyClient::parsHttpProxy client is isBlockHost %1:%2").arg( host.ip.toString() ).arg( host.port ));
			sendResponse( 423, "Locked" );
			slot_stop();
			return;
		}
		app::setLog(4,QString("ProxyClient::parsHttpProxy request connect [%1]->[%2:%3]").arg(m_userLogin).arg(host.ip.toString()).arg(host.port));

		m_pTarget->connectToHost( host.ip, host.port );
		m_pTarget->waitForConnected( 1300 );

		if( m_pTarget->isOpen() ){
			app::setLog( 4,QString("ProxyClient::parsHttpProxy connection accepted").arg( m_userLogin ));
			if( host.ip.toString() == "127.0.0.1" && host.port == app::conf.controlPort && m_auth ){
				app::setLog(5,QString("ProxyClient::Send auth data from control server [%1]").arg( m_userLogin ));
				QByteArray ba;
				ba.append( Control::AUTH );
				ba.append( mf::toBigEndianShort( static_cast< int16_t >( m_userLogin.length() ) ) );
				ba.append( m_userLogin );
				ba.append( mf::toBigEndianShort( static_cast< int16_t >( m_userPass.length() ) ) );
				ba.append( m_userPass );
				ba.append( '\0' );
				sendToTarget( ba );
			}

			m_tunnel = true;

			if( m_proto == Client::Proto::HTTPS ){
				sendRawResponse( 200, "Connection established", "", "" );
				break;
			}

			if( m_proto == Client::Proto::HTTP ){
				sendToTarget( http::buildPkt(pkt) );
				break;
			}

			break;
		}
		//else sendResponse( 504, "Gateway Timeout" );
	}

	if( !m_tunnel ){
		sendResponse( 502, "Bad Gateway" );
		return;
	}else{
		if( m_proto == Client::Proto::HTTP )	m_targetHostStr = QString("http://%1").arg( m_targetHostStr );
		if( m_proto == Client::Proto::HTTPS )	m_targetHostStr = QString("https://%1%2").arg( m_targetHostStr ).arg( pkt.head.request.target );
		app::addUserConnection( m_userLogin, m_targetHostStr );
	}
}

void Client::parsAuth(const QString &string, const QString &method)
{
	app::setLog(5,QString("ProxyClient::parsAuth [%1]").arg(string));

	if( m_proto == Client::Proto::UNKNOWN ){
		app::setLog(2,QString("ProxyClient::parsAuth unknown proto"));
		return;
	}

	if( m_proto == Client::Proto::HTTP || m_proto == Client::Proto::HTTPS ){
		auto auth = http::parsAuthString( string.toUtf8() );
		app::setLog(4,QString("ProxyClient::parsAuth method %1").arg(auth.method));

		if( auth.method == http::AuthMethod::Basic && app::conf.httpAuthMethod == http::AuthMethod::Basic ){
			QByteArray decodeStr = QByteArray::fromBase64( auth.BasicString );
			auto tmp = decodeStr.split(':');
			if( tmp.size() != 2 ) return;
			auth.username = tmp[0];
			m_userPass = tmp[1];
			if( app::chkAuth( auth.username, m_userPass ) ){
				app::setLog(4,QString("ProxyClient::parsAuth %1 auth true").arg(auth.username));
				m_userLogin = auth.username;
				app::updateUserLoginTimeStamp( auth.username );
				m_auth = true;
			}

			return;
		}

		if( auth.method == http::AuthMethod::Digest && app::conf.httpAuthMethod == http::AuthMethod::Digest ){
			QByteArray HA1 = app::getHA1Code( auth.username );
			QByteArray HA2;
			QByteArray response;

			if( auth.qop == "auth-int" ){
				//TODO: Доделать
				//HA2 = mf::md5( method.toUtf8() + ":" + auth.uri + ":" + mf::md5(<entityBody>).toHex() ).toHex();
				//response = mf::md5( HA1 + ":" + app::getNonceCode() + ":" + auth.nc.toUtf8() + ":" + auth.cnonce.toUtf8() + ":" + auth.qop.toUtf8() + ":" + HA2  ).toHex();
			}
			if( auth.qop == "auth" ){
				HA2 = mf::md5( method.toUtf8() + ":" + auth.uri.toUtf8() ).toHex();
				response = mf::md5( HA1 + ":" + app::getNonceCode() + ":" + auth.nc.toUtf8() + ":" + auth.cnonce.toUtf8() + ":" + auth.qop.toUtf8() + ":" + HA2  ).toHex();
			}
			if( auth.qop != "auth" && auth.qop != "auth-int" ){
				HA2 = mf::md5( method.toUtf8() + ":" + auth.uri.toUtf8() ).toHex();
				response = mf::md5( HA1 + ":" + app::getNonceCode() + ":" + HA2  ).toHex();
			}

			if( response == auth.response ){
				app::setLog(4,QString("ProxyClient::parsAuth %1 auth true").arg(auth.username));
				m_userLogin = auth.username;
				m_userPass = app::getUserPass(m_userLogin);
				app::updateUserLoginTimeStamp( auth.username );
				m_auth = true;
			}

			return;
		}
	}
}

void Client::sendNoAuth()
{
	if( m_proto == Client::Proto::UNKNOWN ){
		app::setLog(2,QString("ProxyClient::sendNoAuth unknown proto"));
		return;
	}

	app::setLog(4,QString("ProxyClient::sendNoAuth [%1]").arg( m_pClient->peerAddress().toString() ));

	if( m_proto == Client::Proto::HTTP || m_proto == Client::Proto::HTTPS ){
		http::pkt pkt;
		pkt.head.response.code = 407;
		pkt.head.response.comment = "Proxy Authentication Required";
		pkt.head.proxyAuthenticate = app::getHttpAuthString();
		pkt.body.rawData.append( app::getHtmlPage("Service page","<h1>Proxy Authentication Required</h1>") );
		pkt.head.connection = "keep-alive";
		sendToClient( http::buildPkt(pkt) );
	}
}

void Client::sendNoAccess()
{
	if( m_proto == Client::Proto::UNKNOWN ){
		app::setLog(2,QString("ProxyClient::sendNoAccess unknown proto"));
		return;
	}

	if( m_proto == Client::Proto::HTTP || m_proto == Client::Proto::HTTPS ){
		http::pkt pkt;
		pkt.head.response.code = 423;
		pkt.head.response.comment = "Locked";
		QByteArray ba;
		ba.append("<h1>Too many failed authorization attempts, access for your IP is blocked for another 60 seconds. Total blocking time: ");
		ba.append( QString::number( app::getTimeBan( m_pClient->peerAddress() ) ) );
		ba.append("sec.</h1>");
		pkt.body.rawData.append( app::getHtmlPage("Service page", ba) );
		sendToClient( http::buildPkt(pkt) );
		app::addBAN( m_pClient->peerAddress() );
		app::setLog(3,QString("ProxyClient::sendNoAccess [%1]").arg(m_pClient->peerAddress().toString()));
		slot_stop();
	}
}

void Client::moveToLockedPage(const QString &reffer)
{
	if( m_proto == Client::Proto::UNKNOWN ){
		app::setLog(2,QString("ProxyClient::moveToLockedPage unknown proto"));
		return;
	}

	if( m_proto == Client::Proto::HTTP || m_proto == Client::Proto::HTTPS ){
		http::pkt pkt;
		pkt.head.response.code = 301;
		pkt.head.response.comment = "Moved Permanently";
		pkt.head.location = QString("0.0.0.0:%1/locked").arg( app::conf.controlPort );
		pkt.head.reffer = reffer;
		pkt.head.connection = "keep-alive";
		sendToClient( http::buildPkt(pkt) );

		app::setLog(3,QString("ProxyClient::moveToLockedPage [%1]").arg(m_pClient->peerAddress().toString()));
	}
}
