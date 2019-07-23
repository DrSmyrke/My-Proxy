#include "client.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QUrl>

//TODO: remove qDebug
#include <QDebug>

Client::Client(QObject *parent) : QObject(parent)
{
	m_pClient = new QTcpSocket(this);
	m_pTarget = new QTcpSocket(this);



	connect( m_pClient, &QTcpSocket::readyRead, this, &Client::slot_clientReadyRead );
	connect( m_pTarget, &QTcpSocket::readyRead, this, &Client::slot_targetReadyRead );
	connect( m_pTarget, &QTcpSocket::disconnected, this, &Client::slot_targetDisconnected );
	connect( m_pClient, &QTcpSocket::disconnected, this, &Client::slot_clientDisconnected );
}

void Client::setSocketDescriptor(qintptr socketDescriptor)
{
	m_pClient->setSocketDescriptor( socketDescriptor );
}

void Client::stop()
{
	m_pClient->close();
	m_finished = true;
	emit signal_finished();
}

void Client::slot_clientReadyRead()
{
	QByteArray buff;
	while( m_pClient->bytesAvailable() ){
		QByteArray tmp = m_pClient->read(1);
		buff.append( tmp );
		if( m_pTarget->isOpen() ){
			sendToTarget( buff );
			buff.clear();
		}
	}
	app::setLog(5,QString("Client::slot_clientReadyRead %1 bytes [%2]").arg(buff.size()).arg(QString(buff)));

	//qDebug()<<buff;

	//TODO: Обработка блокировки и отключения клиента	sendResponse( 423, "Locked" );

	if( m_pTarget->isOpen() ){
		sendToTarget( buff );
		return;
	}

	auto pkt = http::parsPkt( buff );
	if( !pkt.head.valid ){
		sendResponse( 400, "<h1>Bad Request</h1>" );
		return;
	}

	if( !pkt.head.proxyAuthorization.isEmpty() ) parsAuth( pkt.head.proxyAuthorization );
	if( !m_auth ){
		if( failedAuthorization >= app::conf.maxFailedAuthorization ){
			sendNoAccess();
			return;
		}
		failedAuthorization++;
		sendNoAuth();
		return;
	}



	//TODO: Реализовать ограничение колличество соединений на клиента	sendResponse(429,"Too Many Requests");


	QString addr;
	QString path;
	uint16_t port = 80;

	if( pkt.head.valid && pkt.head.isRequest ){
		if( pkt.head.request.method == "CONNECT" ){
			auto tmp = pkt.head.request.target.split(":");
			if( tmp.size() > 0 ) addr = tmp[0];
			if( tmp.size() == 2 ) port = tmp[1].toUInt();
			app::setLog(3,QString("WebProxyClient::slot_clientReadyRead HTTPS Request to [%1]").arg(pkt.head.request.target));
			m_proto = http::Proto::HTTPS;
			//Black list addrs
			if( app::findInBlackList( addr, app::blackList.addrs ) ){
				m_proto = http::Proto::UNKNOW;
				buff.clear();
			}
		}
		if( pkt.head.request.method == "GET" || pkt.head.request.method == "POST" ){
			QUrl url(pkt.head.request.target);
			if( !url.isValid() ){
				sendResponse( 400, "<h1>Bad Request</h1>" );
				return;
			}else{
				addr = url.host();
				port = static_cast<uint16_t>( url.port(port) );
				path = url.path();
				if( !url.query().isEmpty() ) path += "?" + url.query();
				path.replace( " ", "%20" );
			}
			app::setLog(3,QString("WebProxyClient::slot_clientReadyRead HTTP Request to [%1:%2] %3").arg(addr).arg(port).arg(path));
			pkt.head.request.target = path;
			pkt.head.proxyAuthorization.clear();
			//pkt.head.connection = "close";
			buff.clear();
			buff.append( http::buildPkt( pkt ) );
			m_proto = http::Proto::HTTP;
			//dont statistic settings url
			if( pkt.head.host != app::conf.adminkaHostAddr ) app::addOpenUrl(url);
			//Black list urls
			if( app::findInBlackList( url.toString(), app::blackList.urls ) ){
				m_proto = http::Proto::UNKNOW;
				buff.clear();
			}
		}
	}

	if( pkt.head.isRequest && m_proto != http::Proto::UNKNOW ){
		//dont statistic settings url
		if( pkt.head.host != app::conf.adminkaHostAddr ) app::addOpenAddr( addr );
	}







	if( pkt.head.isRequest && m_proto == http::Proto::HTTP ){
		if( pkt.head.host == app::conf.adminkaHostAddr ){
			if( m_pTarget->isOpen() ) m_pTarget->close();
			m_tunnel = false;
			bool error = true;

			if( pkt.head.request.target == "/buttons.css" ){
				sendRawResponse( 200, "OK", app::conf.page.buttonsCSS, "text/css; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target == "/color.css" ){
				sendRawResponse( 200, "OK", app::conf.page.colorCSS, "text/css; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target == "/index.js" ){
				sendRawResponse( 200, "OK", app::conf.page.indexJS, "application/javascript; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target == "/index.css" ){
				sendRawResponse( 200, "OK", app::conf.page.indexCSS, "text/css; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target == "/" ){
				sendResponse( 200, "Test content" );
				error = false;
			}
			if( pkt.head.request.target == "/state" ){
				sendRawResponse( 200, "OK", app::getHtmlPage( "State", app::conf.page.state ), "text/html; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target == "/admin" ){
				sendRawResponse( 200, "OK", app::getHtmlPage( "Admin", app::conf.page.admin ), "text/html; charset=utf-8" );
				error = false;
			}
			if( pkt.head.request.target.indexOf("/get?",Qt::CaseInsensitive) == 0 or pkt.head.request.target.indexOf("/set?",Qt::CaseInsensitive) == 0 ){
				auto response = app::parsRequest( pkt.head.request.target, m_user );
				if( response.size() == 0 ){
					sendRawResponse( 404, "Not found", "", "text/html; charset=utf-8" );
				}else{
					sendRawResponse( 200, "OK", response, "text/html; charset=utf-8" );
				}
				error = false;
			}

			if( error ) sendResponse( 502, "<h1>Bad Gateway</h1>" );
			return;
		}
		//qDebug()<<http::buildPkt( pkt );
		//qDebug()<<pkt.head.request.target<<pkt.head.host;
	}













	if( m_proto == http::Proto::UNKNOW ){
		sendResponse( 400, "<h1>Bad Request</h1>" );
		if( m_pTarget->isOpen() ) m_pTarget->close();
		return;
	}

	if( m_pTarget->isOpen() ){
		sendToTarget( buff );
	}else{
		app::setLog(4,QString("WebProxyClient::slot_clientReadyRead Connecting to [%1:%2]").arg(addr).arg(port));
		auto info = QHostInfo::fromName( addr );

		if( info.error() == QHostInfo::NoError ){
			//TODO: Реализовать фильтрацию траффика
			for(auto elem:info.addresses()){
				app::setLog(5,QString("WebProxyClient::slot_clientReadyRead Connecting to [%1:%2]").arg(elem.toString()).arg(port));
				m_pTarget->connectToHost( elem, port );
				m_pTarget->waitForConnected(1000);
				app::setLog(5,QString("WebProxyClient::slot_clientReadyRead Connecting state [%1] is open [%2]").arg(m_pTarget->state()).arg(m_pTarget->isOpen()));
				if( m_pTarget->state() != QAbstractSocket::ConnectedState ) m_pTarget->waitForConnected();
				if( m_pTarget->state() != QAbstractSocket::ConnectedState ) break;
				if( m_pTarget->isOpen() ){
					if( m_pTarget->state() != QAbstractSocket::ConnectedState ) m_pTarget->waitForConnected();
					if( m_proto == http::Proto::HTTPS ){
						m_tunnel = true;
						sendResponse( 200, "<h1>Connection established</h1>" );
					}
					if( m_proto == http::Proto::HTTP ){
						m_pTarget->write( buff );
						m_pTarget->waitForBytesWritten(100);
					}
					break;
				}
			}
			if( !m_pTarget->isOpen() ){
				sendResponse( 504, "<h1>Gateway Timeout</h1>" );
				return;
			}
		}else{
			sendResponse( 502, "<h1>Bad Gateway</h1>" );
		}
	}
}

void Client::slot_targetReadyRead()
{
	QByteArray buff;
	if( m_proto == http::Proto::HTTPS ){
		qint64 len = m_pTarget->bytesAvailable();
		while( m_pTarget->bytesAvailable() ) sendToClient( m_pTarget->read(1) );//buff.append( m_pTarget->readAll() );
		app::setLog(5,QString("WebProxyClient::slot_targetReadyRead %1 bytes [%2]").arg(len).arg(QString(buff)));

		//TODO: block list start

		//block list end


	}
	if( m_proto == http::Proto::HTTP ){
		//TODO: Реализовать фильтрацию трафика
		while( m_pTarget->bytesAvailable() ) buff.append( m_pTarget->readAll() );
		app::setLog(5,QString("WebProxyClient::slot_targetReadyRead %1 bytes [%2]").arg(buff.size()).arg(QString(buff)));

		//TODO: block list start

		//block list end

		//qDebug()<<"slot_targetReadyRead"<<buff;

		sendToClient( buff );
	}
}

void Client::slot_targetDisconnected()
{
	app::setLog(5,QString("Client::Target disconnected"));
	stop();
}

void Client::slot_clientDisconnected()
{
	app::setLog(5,QString("Client::Client disconnected"));
	if( m_pTarget->isOpen() ) m_pTarget->close();
	stop();
}

void Client::sendResponse(const uint16_t code, const QString &comment)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	pkt.body.rawData.append( app::getHtmlPage("Service page",comment.toLatin1()) );
	app::setLog(3,QString("WebProxyClient::sendResponse [%1]").arg(pkt.head.response.code));
	sendToClient( http::buildPkt(pkt) );
}

void Client::sendRawResponse(const uint16_t code, const QString &comment, const QString &data, const QString &mimeType)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	if( !mimeType.isEmpty() ) pkt.head.contType = mimeType;
	pkt.body.rawData.append( data );
	app::setLog(3,QString("WebProxyClient::sendRawResponse [%1]").arg(pkt.head.response.code));
	sendToClient( http::buildPkt(pkt) );
}

void Client::sendNoAuth()
{
	http::pkt pkt;
	pkt.head.response.code = 407;
	pkt.head.response.comment = "Proxy Authentication Required";
	//TODO: Доделать прочие типы авторизаций
	switch (app::conf.authMetod){
		case  http::AuthMethod::Basic:	pkt.head.proxyAuthenticate = "Basic realm=ProxyAuth";	break;
		//case  http::AuthMethod::Digest:	pkt.head.proxyAuthenticate = "Digest realm=ProxyAuth";	break;
	}
	pkt.body.rawData.append( app::getHtmlPage("Service page","<h1>Proxy Authentication Required</h1>") );
	sendToClient( http::buildPkt(pkt) );
}

void Client::sendNoAccess()
{
	http::pkt pkt;
	pkt.head.response.code = 200;
	pkt.head.response.comment = "Forbidden";
	//TODO: Сделать вывод общего времени блокировки
	pkt.body.rawData.append( app::getHtmlPage("Service page","<h1>Too many failed authorization attempts, access for your IP is blocked for another 30 minutes. Total blocking time:</h1>") );
	sendToClient( http::buildPkt(pkt) );
	//TODO: Сделать блокировку по IP еще на 30 минут
}

void Client::sendToClient(const QByteArray &data)
{
	if( m_pClient->state() != QAbstractSocket::ConnectedState ) m_pClient->waitForConnected();
	if( m_pClient->state() != QAbstractSocket::ConnectedState ) return;
	m_pClient->write(data);
	m_pClient->waitForBytesWritten(100);
}

void Client::sendToTarget(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pTarget->state() != QAbstractSocket::ConnectedState ) m_pTarget->waitForConnected();
	if( m_pTarget->state() != QAbstractSocket::ConnectedState ) return;
	if( m_proto == http::Proto::HTTPS ){
		m_pTarget->write( data );
	}
	if( m_proto == http::Proto::HTTP ){
		//TODO: Реализовать подмену заголовка и анонимный режим (пересборка пакета)
		m_pTarget->write( data );
	}
	m_pTarget->waitForBytesWritten(100);
}

void Client::parsAuth(const QString &string)
{
	app::setLog(3,QString("Client::parsAuth [%1]").arg(string));

	auto tmp = string.split(" ");
	if( tmp.size() != 2 ) return;
	QString method = tmp[0];
	QString str = tmp[1];
	if( method == "Basic" ){
		QString decodeStr = QByteArray::fromBase64( str.toUtf8() );
		tmp = decodeStr.split(":");
		if( tmp.size() != 2 ) return;
		QString login = tmp[0];
		QString pass = tmp[1];
		if( app::chkAuth( login, pass ) ){
			app::setLog(4,QString("WebProxyClient::parsAuth %1 auth true").arg(login));
			app::getUserData( m_user, login );
			m_auth = true;
			emit signal_authOK();
		}
	}
}
