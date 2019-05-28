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
	app::setLog(5,QString("Client::slot_clientReadyRead %1 bytes [%2]").arg(buff.size()).arg(QString(buff)));

	qDebug()<<buff.toHex();
	//"CONNECT cash4brands.ru:443 HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:67.0) Gecko/20100101 Firefox/67.0\r\nProxy-Connection: keep-alive\r\nConnection: keep-alive\r\nHost: cash4brands.ru:443\r\n\r\n"

	auto pkt = http::parsPkt( buff );
	if( !pkt.head.valid ){
		sendResponse( 400, "Bad Request" );
		return;
	}
	if( !pkt.head.proxyAuthorization.isEmpty() ) parsAuth( pkt.head.proxyAuthorization );
	if( !m_auth ){
		sendNoAuth();
		return;
	}
}

void Client::sendResponse(const uint16_t code, const QString &comment)
{
	http::pkt pkt;
	pkt.head.response.code = code;
	pkt.head.response.comment = comment;
	pkt.body.rawData.append( app::getHtmlPage("Service page","<h1>" + comment + "</h1>") );
	app::setLog(3,QString("WebProxyClient::sendResponse [%1] %2").arg(code).arg(comment));
	sendToClient( app::genHttpHeader(pkt) );
}

void Client::sendNoAuth()
{
	HTTP_Pkt pkt;
	pkt.head.response.code = 407;
	pkt.head.response.comment = "Proxy Authentication Required";
	//TODO: Доделать прочие типы авторизаций
	switch (app::conf.authMetod){
		case  AuthMethod::Basic:	pkt.head.proxyAuthenticate = "Basic realm=ProxyAuth";	break;
		//case  AuthMethod::Digest:	pkt.head.proxyAuthenticate = "Digest realm=ProxyAuth";	break;
	}
	pkt.body.rawData.append( app::getHtmlPage("Service page","<h1>Proxy Authentication Required</h1>") );
	sendToClient( app::genHttpHeader(pkt) );
}

void Client::sendToClient(const QByteArray &data)
{
	m_pClient->write(data);
	m_pClient->waitForBytesWritten(100);
}
