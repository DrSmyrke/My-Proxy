#include "controlserver.h"

ControlServer::ControlServer(QObject *parent) : QTcpServer(parent)
{
	app::setLog( 0, QString("CONTROL SERVER CREATING v%1 ...").arg(app::conf.version) );
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
{
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

	app::setLog(0,QString("ControlClient::slot_clientReadyRead %1 bytes [%2] [%3]").arg(buff.size()).arg(QString(buff)).arg(QString(buff.toHex())));
}
