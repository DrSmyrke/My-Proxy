#include "controlserver.h"

ControlServer::ControlServer(QObject *parent) : QTcpServer(parent)
{
	app::setLog( 0, QString("CONTROL SERVER CREATING ...") );
}

ControlServer::~ControlServer()
{
	app::setLog( 0, "CONTROL SERVER DIE..." );
	emit signal_stopAll();
}

bool ControlServer::run()
{
	if(!this->listen( QHostAddress::LocalHost, app::conf.controlPort )){
		app::setLog( 0, QString("CONTROL SERVER [ NOT ACTIVATED ] PORT: [%1] %2").arg( app::conf.controlPort ).arg( this->errorString() ) );
		return false;
	}

	app::setLog( 0, QString("CONTROL SERVER [ ACTIVATED ] PORT: [%1]").arg( app::conf.controlPort ) );

	return true;
}

void ControlServer::stop()
{
	app::setLog( 0, "CONTROL SERVER STOPPING..." );
	app::saveSettings();
	this->close();
}

void ControlServer::incomingConnection(qintptr socketDescriptor)
{
	ControlClient* client = new ControlClient(socketDescriptor);
	//QThread* thread = new QThread();
	//client->moveToThread(thread);
//	connect(thread,&QThread::started,client,&Client::slot_start);
//	connect(client,&Client::signal_finished,thread,&QThread::quit);
	connect(this,&ControlServer::signal_stopAll,client,&ControlClient::slot_stop);
	connect(client,&ControlClient::signal_finished,client,&ControlClient::deleteLater);
//	connect(thread,&QThread::finished,thread,&QThread::deleteLater);
//	thread->start();
	client->run();
}




/// #####################################################################
/// #																	#
/// #						Control Client								#
/// #																	#
/// #####################################################################



ControlClient::ControlClient(qintptr descriptor, QObject *parent)
	: QObject(parent)
	, m_auth(false)
{
	app::setLog(5,QString("ControlClient::ControlClient created"));

	m_pClient = new QTcpSocket();

	if( !m_pClient->setSocketDescriptor( descriptor ) ) slot_stop();

	connect( m_pClient, &QTcpSocket::readyRead, this, &ControlClient::slot_clientReadyRead);
	connect( m_pClient, &QTcpSocket::disconnected, this, [this](){
		app::setLog(5,QString("ControlClient::ControlClient disconnected"));
		slot_stop();
	});
}

void ControlClient::slot_start()
{

}

void ControlClient::slot_stop()
{
	if( m_pClient->isOpen() ) m_pClient->close();
	if( m_auth ) app::changeUserConnection( m_user.login, -1 );
	emit signal_finished();
}

void ControlClient::slot_clientReadyRead()
{
	QByteArray buff;

	if( !m_pClient->isReadable() ) m_pClient->waitForReadyRead(300);

	while( m_pClient->bytesAvailable() ){
		buff.append( m_pClient->read(1024) );
	}

	app::setLog(5,QString("ControlClient::slot_clientReadyRead %1 bytes [%2] [%3]").arg(buff.size()).arg(QString(buff)).arg(QString(buff.toHex())));

	// Если придет запрос на получение информации
	QByteArray ba;
	if( parsInfoPkt( buff, ba ) ){
		sendToClient(ba);
		return;
	}

	if( !m_auth ) parsAuthPkt( buff );
	if( !m_auth ){
		slot_stop();
		return;
	}

	http::pkt pkt;

	if( m_buff.size() > 0 ){
		m_buff.append( buff );
		pkt = http::parsPkt( m_buff );
	}else{
		pkt = http::parsPkt( buff );
	}
	if( pkt.next ){
		m_buff.append( buff );
		return;
	}
	if( pkt.valid ) processingRequest( pkt );
}

void ControlClient::sendToClient(const QByteArray &data)
{
	if( data.size() == 0 ) return;
	if( m_pClient->state() == QAbstractSocket::ConnectingState ) m_pClient->waitForConnected(300);
	if( m_pClient->state() == QAbstractSocket::UnconnectedState ) return;
	m_pClient->write(data);
	m_pClient->waitForBytesWritten(100);
	app::setLog(5,QString("Client::sendToClient %1 bytes [%2]").arg(data.size()).arg(QString(data.toHex())));
}

bool ControlClient::parsAuthPkt(QByteArray &data)
{
	bool res = false;

	char cmd = data[0];
	data.remove( 0, 1 );

	uint16_t len = 0;
	QByteArray login;
	QByteArray pass;

	switch( cmd ){
		case Control::AUTH:
			len = data[0] << 8;
			len += data[1];
			data.remove( 0, 2 );

			login = data.left( len );
			data.remove( 0, len );

			len = data[0] << 8;
			len += data[1];
			data.remove( 0, 2 );

			pass = data.left( len );
			data.remove( 0, len );

			app::setLog(5,QString("ControlClient::parsAuthPkt recv [%1:%2]").arg(QString(login)).arg(QString(pass)));

			if( data[0] == '\0' ) res = true;
			data.remove( 0, 1 );

			if( app::chkAuth2( login, pass ) ){
				m_user = app::getUserData( login );
				m_auth = true;
				app::setLog(4,QString("ControlClient::parsAuthPkt() auth success [%1]").arg(QString(login)));
			}else{
				app::setLog(3,QString("ControlClient::parsAuthPkt() auth error [%1:%2]").arg(QString(login)).arg(QString(pass)));
			}

		break;
	}

	return res;
}

bool ControlClient::parsInfoPkt(QByteArray &data, QByteArray &sendData)
{
	bool res = false;

	if( data.size() < 2 ) return res;

	char cmd = data[0];
	char param = data[1];

	if( cmd != Control::INFO ) return res;

	app::setLog(4,QString("ControlClient::parsInfoPkt() [%1]").arg(QString(data.toHex())));

	res = true;
	sendData.clear();
	QString str;
	QStringList list;

	switch( param ){
		case Control::VERSION:
			sendData.append( app::conf.version );
		break;
		case Control::USERS:
			list.clear();
			for( auto user:app::conf.users ){
				str = QString("%1	%2	%3	%4	%5	%6	%7	%8").arg( user.login ).arg( app::conf.usersConnections[user.login] ).arg( user.maxConnections ).arg( user.lastLoginTimestamp ).arg( mf::getSize( user.inBytes ) ).arg( mf::getSize( user.inBytesMax ) ).arg( mf::getSize( user.outBytes ) ).arg( mf::getSize( user.outBytesMax ) );
				list.push_back( str );
			}
			sendData.append( list.join( ";" ) );
		break;
		case Control::BLACKLIST_ADDRS:
			list.clear();
			for( auto addr:app::accessList.blackDomains ) list.push_back( addr );
			sendData.append( list.join( ";" ) );
		break;
	}

	return res;
}

void ControlClient::sendRawResponse(const uint16_t code, const QString &comment, const QString &data, const QString &mimeType)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	if( !mimeType.isEmpty() ) pkt.head.contType = mimeType;
	pkt.body.rawData.append( data );
	app::setLog( 4, QString("ControlClient::sendRawResponse [%1]").arg(pkt.head.response.code) );
	sendToClient( http::buildPkt(pkt) );
}

void ControlClient::sendResponse(const uint16_t code, const QString &comment)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	pkt.body.rawData.append( app::getHtmlPage("Service page",comment.toLatin1()) );
	app::setLog( 4, QString("ControlClient::sendResponse [%1]").arg(pkt.head.response.code) );
	sendToClient( http::buildPkt(pkt) );
}

void ControlClient::processingRequest(const http::pkt &pkt)
{
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
	if( pkt.head.request.target == "/down-arrow.png" ){
		sendRawResponse( 200, "OK", app::conf.page.downArrowIMG, "image/png" );
		error = false;
	}
	if( pkt.head.request.target == "/up-arrow.png" ){
		sendRawResponse( 200, "OK", app::conf.page.upArrowIMG, "image/png" );
		error = false;
	}
	if( pkt.head.request.target == "/" ){
		sendRawResponse( 200, "OK", app::getHtmlPage( "Index", app::conf.page.index ), "text/html; charset=utf-8" );
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
		QMap<QByteArray, QByteArray> args;
		http::parsArguments( pkt.head.request.target, args );
		auto response = app::processingRequest( pkt.head.request.method, args, m_user );
		if( response.size() == 0 ){
			sendRawResponse( 404, "Not found", "", "text/html; charset=utf-8" );
		}else{
			sendRawResponse( 200, "OK", response, "text/html; charset=utf-8" );
		}
		error = false;
	}

	if( error ) sendResponse( 502, "<h1>Bad Gateway</h1>" );


//	if( m_auth ){
//		if( m_user.group == UserGrpup::admins ){
//			if( pkt.head.isRequest && pkt.head.request.target == "/reloadSettings" ){
//				app::loadSettings();
//				QString content = "<script>document.location.href=\"/\";</script><meta http-equiv=\"Refresh\" content=\"0; URL=/\">";
//				sendResponse( app::getHtmlPage( content ).toUtf8(), 200 );
//				return;
//			}
//		}



//		QString content;

//		content += QString("Hi %1 [%2] v%3<br>\n").arg( m_user.login ).arg( app::getUserGroupNameFromID( m_user.group ) ).arg( app::conf.version );
//		if( m_user.outBytes >= m_user.outBytesMax ){
//			content += "<h2>Outgoing traffic has ended :(</h2><br>\n";
//		}
//		if( m_user.inBytes >= m_user.inBytesMax ){
//			content += "<h2>Incoming traffic has ended :(</h2><br>\n";
//		}
//		content += "============  TRAFFIC  =========================<br>\n";
//		content += QString("OUT: %1 / %2<br>\n").arg( mf::getSize( m_user.outBytes ) ).arg( mf::getSize( m_user.outBytesMax ) );
//		content += QString("IN:  %1 / %2<br>\n").arg( mf::getSize( m_user.inBytes ) ).arg( mf::getSize( m_user.inBytesMax ) );
//		content += "<hr>";
//		if( m_user.group == UserGrpup::admins ){
//			content += "<a href=\"/reloadSettings\">reloadSetting</a>";
//			content += "<hr>";
//		}

//		content += "============  USERS  =========================<br>\n";
//		for( auto user:app::conf.users ){
//			content += QString("%1	%2/%3 in:[%4/%5] out:[%6/%7]	%8<br>\n").arg( user.login ).arg( app::conf.usersConnections[user.login] ).arg( user.maxConnections ).arg( mf::getSize( user.inBytes ) ).arg( mf::getSize( user.inBytesMax ) ).arg( mf::getSize( user.outBytes ) ).arg( mf::getSize( user.outBytesMax ) ).arg( user.lastLoginTimestamp );
//		}
//		//content += "============  BLACK DYNAMIC  =========================<br>\n";
//		//for( auto elem:app::accessList.blackIPsDynamic ){
//		//	content += QString("%1:%2<br>\n").arg( elem.ip.toString() ).arg( elem.port );
//		//}

//		sendResponse( app::getHtmlPage( content ).toUtf8(), 200 );
//	}else{
//		QString content = "You are not auth!!! :(";
//		sendResponse( app::getHtmlPage( content ).toUtf8(), 200 );
//	}
}