#ifndef CONTROLSERVER_H
#define CONTROLSERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QTcpSocket>

#include "global.h"

class ControlServer : public QTcpServer
{
	Q_OBJECT
public:
	ControlServer(QObject *parent = 0);
	~ControlServer();
	bool run();
	void stop();
protected:
	void incomingConnection(qintptr socketDescriptor);
signals:
	void signal_stopAll();
};

class ControlClient : public QObject
{
	Q_OBJECT
public:
	explicit ControlClient(qintptr descriptor, QObject *parent = 0);
	void run() { slot_start(); }
signals:
	void signal_finished();
public slots:
	void slot_start();
	void slot_stop();
private slots:
	void slot_clientReadyRead();
private:
	QTcpSocket* m_pClient;
	bool m_auth;
	User m_user;

//	void sendError(const uint8_t protoByte, const QString &errorString = QString(""), const uint8_t errorCode = 0x05, const uint8_t level = 4);
//	void log(const QString &text);
//	void sendToClient(const QByteArray &data);
//	void sendToTarget(const QByteArray &data);
	bool parsAuthPkt(QByteArray &data);
//	bool parsAuthMethods(QByteArray &data);
//	bool parsConnectPkt(QByteArray &data, QHostAddress &addr, uint16_t &port, QString &domainAddr);
//	void parsIP(QByteArray &data, QHostAddress &addr);
//	void parsPORT(QByteArray &data, uint16_t &port);
	void parsAdminPkt(QByteArray &data, QByteArray &sendData);
};

#endif // CONTROLSERVER_H
