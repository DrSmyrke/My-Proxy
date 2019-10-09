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
	if( m_auth ) app::changeUserConnection( m_userLogin, -1 );
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
		app::setLog( 5, QString("ControlClient::slot_clientReadyRead packet small") );
		m_buff.append( buff );
		return;
	}
	if( pkt.valid ){
		processingRequest( pkt );
	}else{
		app::setLog( 5, QString("ControlClient::slot_clientReadyRead packet is NOT valid [%1][%2][%3]").arg( QString( buff ) ).arg( pkt.head.valid ).arg( pkt.body.valid ) );
	}
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
				m_userLogin = login;
				m_auth = true;
				app::setLog(4,QString("ControlClient::parsAuthPkt auth success [%1]").arg(QString(login)));
			}else{
				app::setLog(2,QString("ControlClient::parsAuthPkt auth error [%1:%2]").arg(QString(login)).arg(QString(pass)));
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

void ControlClient::sendRawResponse(const uint16_t code, const QString &comment, const QByteArray &data, const QString &mimeType)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	if( !mimeType.isEmpty() ) pkt.head.contType = mimeType;
	pkt.body.rawData.append( data );
	app::setLog( 5, QString("ControlClient::sendRawResponse [%1]").arg(pkt.head.response.code) );
	sendToClient( http::buildPkt(pkt) );
}

void ControlClient::sendResponse(const uint16_t code, const QString &comment)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	pkt.body.rawData.append( app::getHtmlPage("Service page",comment.toLatin1()) );
	app::setLog( 5, QString("ControlClient::sendResponse [%1]").arg(pkt.head.response.code) );
	sendToClient( http::buildPkt(pkt) );
}

void ControlClient::processingRequest(const http::pkt &pkt)
{
	bool error = true;
	QString method;

	app::setLog( 5, QString("ControlClient::processingRequest [%1]").arg( pkt.head.request.target ) );

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
		return;
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
	if( pkt.head.request.target == "/config" ){
		sendRawResponse( 200, "OK", app::getHtmlPage( "Config", app::conf.page.config ), "text/html; charset=utf-8" );
		error = false;
	}
	if( pkt.head.request.target.indexOf("/get?",Qt::CaseInsensitive) == 0 ) method = "GET";
	if( pkt.head.request.target.indexOf("/set?",Qt::CaseInsensitive) == 0 ) method = "SET";
	if( method == "GET" || method == "SET" ){
		QMap<QByteArray, QByteArray> args;
		http::parsArguments( pkt.head.request.target, args );
		QByteArray response;
		User myData = app::getUserData( m_userLogin );

		//if( pkt.head.request.method == "GET" ){
		if( method == "GET" ){
			response.append( "content" );
			for( auto param:args.keys() ){
				auto value = args.value( param );
				if( param == "userData" ){
					response.append( ":>:" );
					User userData = app::getUserData( value );

					QString changePass;
					QString maxConnections;
					QString inBytesMax;
					QString outBytesMax;

					if( m_auth && myData.group == UserGrpup::admins ){
						changePass += QString("<form class=\"form\" action=\"/set\" onSubmit=\"return changeParam( this, \'alertBoxPass\', true );\">");
						changePass += QString("<input type=\"hidden\" name=\"user\" value=\"%1\">").arg( QString(value) );
						changePass += QString("<input type=\"password\" name=\"newPass\"> <div style=\"display: inline-block;\" id=\"alertBoxPass\"></div>");
						changePass += QString("</form>");
					}

					maxConnections += QString("<form class=\"form\" action=\"/set\" onSubmit=\"return changeParam( this, \'alertBoxMaxConnections\', true );\">");
					maxConnections += QString("<input type=\"hidden\" name=\"user\" value=\"%1\">").arg( QString(value) );
					maxConnections += QString("<input type=\"text\" name=\"newMaxConnections\" value=\"%1\"> <div style=\"display: inline-block;\" id=\"alertBoxMaxConnections\"></div>").arg( userData.maxConnections );
					maxConnections += QString("</form>");

					inBytesMax += QString("<form class=\"form\" action=\"/set\" onSubmit=\"return changeParam( this, \'alertBoxinBytesMax\', true );\">");
					inBytesMax += QString("<input type=\"hidden\" name=\"user\" value=\"%1\">").arg( QString(value) );
					inBytesMax += QString("<input type=\"text\" name=\"newinBytesMax\" value=\"%1\"> <div style=\"display: inline-block;\" id=\"alertBoxinBytesMax\"></div>").arg( userData.inBytesMax );
					inBytesMax += QString("</form>");

					outBytesMax += QString("<form class=\"form\" action=\"/set\" onSubmit=\"return changeParam( this, \'alertBoxoutBytesMax\', true );\">");
					outBytesMax += QString("<input type=\"hidden\" name=\"user\" value=\"%1\">").arg( QString(value) );
					outBytesMax += QString("<input type=\"text\" name=\"newoutBytesMax\" value=\"%1\"> <div style=\"display: inline-block;\" id=\"alertBoxoutBytesMax\"></div>").arg( userData.outBytesMax );
					outBytesMax += QString("</form>");

					response.append("<table>");
					response.append( QString("<tr><td width=\"120px\">Login:</td><td>%1</td></tr>").arg( userData.login ) );
					if( m_auth && myData.group == UserGrpup::admins ){
						response.append( QString("<tr><td>Password:</td><td>%1</td></tr>").arg( changePass ) );
					}
					response.append( QString("<tr><td>MaxConnections:</td><td>%1</td></tr>").arg( maxConnections ) );
					response.append( QString("<tr><td>inBytesMax:</td><td>%1</td></tr>").arg( inBytesMax ) );
					response.append( QString("<tr><td>outBytesMax:</td><td>%1</td></tr>").arg( outBytesMax ) );
					response.append("</table>");
				}
				if( param == "ss" && value == "serverSettings" && m_auth && myData.group == UserGrpup::admins ){
					response.append( ":>:" );
					response.append( value );
					response.append( ":>:" );

					QString logLevel;
					if( m_auth && myData.group == UserGrpup::admins ){
						logLevel += QString("<form class=\"form\" action=\"/set\" onSubmit=\"return changeParam( this, \'alertLogLevel\', true );\">");
						logLevel += QString("<input type=\"hidden\" name=\"sysParam\" value=\"logLevel\">");
						logLevel += QString("<input type=\"number\" name=\"value\" max=\"6\" min=\"0\" value=\"%1\"> <div style=\"display: inline-block;\" id=\"alertLogLevel\"></div>").arg( app::conf.logLevel );
						logLevel += QString("</form>");
					}else{
						logLevel += QString("%1").arg( app::conf.logLevel );
					}

					response.append("<table>");
					response.append( QString("<tr><td>LogLevel:</td><td>%1</td></tr>").arg( logLevel ) );
					//response.append( QString("<tr><td>MaxConnections:</td><td>%1</td></tr>").arg( maxConnections ) );
					//response.append( QString("<tr><td>inBytesMax:</td><td>%1</td></tr>").arg( inBytesMax ) );
					//response.append( QString("<tr><td>outBytesMax:</td><td>%1</td></tr>").arg( outBytesMax ) );
					response.append("</table>");

					continue;
				}
				if( param == "ud" && value == "usersData" ){
					response.append( ":>:" );
					response.append( value );
					response.append( ":>:" );
					response.append("<table>");
					for( auto user:app::conf.users ){
						QString editB = "";
						if( m_auth && myData.group == UserGrpup::admins ){
							editB = QString("<input type=\"button\" value=\"EDIT\" onClick=\"edit('user', '%1');\">").arg( user.login );
						}
						auto str = QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>\n").arg( user.login ).arg( app::getDateTime( user.lastLoginTimestamp ) ).arg( editB );
						response.append( str );
					}
					response.append("</table>");
					continue;
				}
				if( param == "ut" && value == "usersTraffic" ){
					response.append( ":>:" );
					response.append( value );
					response.append( ":>:" );
					response.append("<table>");
					for( auto user:app::conf.users ){
						auto str = QString("<tr><td>%1</td><td>%2/%3</td><td><img src=\"/down-arrow.png\" class=\"traffIco\"> %4/%5</td><td><img src=\"/up-arrow.png\" class=\"traffIco\"> %6/%7</td></tr>\n").arg( user.login ).arg( app::conf.usersConnections[user.login] ).arg( user.maxConnections ).arg( mf::getSize( user.inBytes ) ).arg( mf::getSize( user.inBytesMax ) ).arg( mf::getSize( user.outBytes ) ).arg( mf::getSize( user.outBytesMax ) );
						response.append( str );
					}
					response.append("</table>");
					continue;
				}
				if( param == "md" && value == "myData" ){
					response.append( ":>:" );
					response.append( value );
					response.append( ":>:" );
					auto str = QString("Hi %1 [%2] v%3<br>\n").arg( m_userLogin ).arg( app::getUserGroupNameFromID( myData.group ) ).arg( app::conf.version );
					response.append( str );
					continue;
				}
				if( param == "gl" && value == "globalLog" ){
					response.append( ":>:" );
					response.append( value );
					response.append( ":>:" );
					QFile file;

					file.setFileName( app::conf.logFile );
					if(file.open(QIODevice::ReadOnly)){
						while (!file.atEnd()) response.append( file.readAll() );
						file.close();
					}
					response.replace('\n',"<br>");

					continue;
				}
			}
		}
		if( method == "SET" ){
			if( args.contains( "sysParam" ) && args.contains( "value" ) && m_auth && myData.group == UserGrpup::admins ){
				auto param = args.value( "sysParam" );
				auto value = args.value( "value" );
				bool find = false;

				if( param == "logLevel" ){
					uint8_t level = static_cast<uint8_t>( value.toUShort() );
					if( level > 6 ) level = 6;
					app::conf.logLevel = level;
					app::conf.settingsSave = true;
					find = true;
				}

				if( find ){
					response.append( "<span class=\"valgreen\">Success!</span>" );
				}else{
					response.append( "<span class=\"message\">ERROR</span>" );
				}
			}
			if( args.contains( "newPass" ) && args.contains( "user" ) ){
				auto user = args.value( "user" );
				auto newPass = args.value( "newPass" );
				if( m_auth && myData.group == UserGrpup::admins ){
					if( app::changePassword( user, newPass ) ){
						response.append( "<span class=\"valgreen\">Success!</span>" );
					}else{
						response.append( "<span class=\"message\">ERROR</span>" );
					}
				}else{
					response.append( "<span class=\"message\">ERROR</span>" );
				}
			}

			if( args.contains( "newMaxConnections" ) && args.contains( "user" ) ){
				auto user = args.value( "user" );
				auto newMaxConnections = args.value( "newMaxConnections" );
				if( m_auth && myData.group == UserGrpup::admins ){
					if( app::changeMaxConnections( user, newMaxConnections.toUInt() ) ){
						response.append( "<span class=\"valgreen\">Success!</span>" );
					}else{
						response.append( "<span class=\"message\">ERROR</span>" );
					}
				}else{
					response.append( "<span class=\"message\">ERROR</span>" );
				}
			}

			if( args.contains( "newinBytesMax" ) && args.contains( "user" ) ){
				auto user = args.value( "user" );
				auto newinBytesMax = args.value( "newinBytesMax" );
				if( m_auth && myData.group == UserGrpup::admins ){
					if( app::changeMaxInBytes( user, newinBytesMax.toUInt() ) ){
						response.append( "<span class=\"valgreen\">Success!</span>" );
					}else{
						response.append( "<span class=\"message\">ERROR</span>" );
					}
				}else{
					response.append( "<span class=\"message\">ERROR</span>" );
				}
			}

			if( args.contains( "newoutBytesMax" ) && args.contains( "user" ) ){
				auto user = args.value( "user" );
				auto newoutBytesMax = args.value( "newoutBytesMax" );
				if( m_auth && myData.group == UserGrpup::admins ){
					if( app::changeMaxOutBytes( user, newoutBytesMax.toUInt() ) ){
						response.append( "<span class=\"valgreen\">Success!</span>" );
					}else{
						response.append( "<span class=\"message\">ERROR</span>" );
					}
				}else{
					response.append( "<span class=\"message\">ERROR</span>" );
				}
			}
		}

		if( response.size() == 0 ){
			sendRawResponse( 404, "Not found", "", "text/html; charset=utf-8" );
		}else{
			sendRawResponse( 200, "OK", response, "text/html; charset=utf-8" );
		}
		error = false;
	}

	if( error ) sendResponse( 502, "<h1>Bad Gateway</h1>" );
}
