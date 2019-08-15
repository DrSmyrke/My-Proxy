#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

#include "global.h"

class Client : public QObject
{
	Q_OBJECT
public:
	struct Pkt{
		uint8_t version;
		uint8_t cmd;
		uint16_t port;
		uint32_t ip;
		QHostAddress addr;
	};
	explicit Client(qintptr descriptor, QObject *parent = 0);
	void run() { slot_start(); }
signals:
	void signal_finished();
public slots:
	void slot_start();
	void slot_stop();
private slots:
	void slot_clientReadyRead();
	void slot_targetReadyRead();
private:
	QTcpSocket* m_pClient;
	QTcpSocket* m_pTarget;
	bool m_tunnel;

	void sendError();
	void log(const QString &text);
	void sendToClient(const QByteArray &data);
	void sendToTarget(const QByteArray &data);
};

#endif // CLIENT_H
