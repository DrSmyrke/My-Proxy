#ifndef CONTROLSERVER_H
#define CONTROLSERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QTcpSocket>

#include "global.h"
#include "http.h"

class ControlServer : public QTcpServer
{
	Q_OBJECT
public:
	ControlServer(QObject *parent = nullptr);
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
	QByteArray m_buff;

	void sendToClient(const QByteArray &data);
	bool parsAuthPkt(QByteArray &data);
	bool parsInfoPkt(QByteArray &data, QByteArray &sendData);
	void sendResponse(const QByteArray &data, const uint16_t code = 200);
	void processingRequest(const http::pkt &pkt);
};

#endif // CONTROLSERVER_H
