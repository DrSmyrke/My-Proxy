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
		bool findAuthLP;
		uint8_t numAuthMethods;
		QByteArray rawRet;
	};
	struct Proto{
		enum Version{
			AUTH_LP		= 0x01,
			SOCKS4		= 0x04,
			SOCKS5		= 0x05,
		};
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
	bool m_auth;

	void sendError(const uint8_t protoByte, const QString &errorString = QString(""), const uint8_t errorCode = 0x05, const uint8_t level = 4);
	void log(const QString &text);
	void sendToClient(const QByteArray &data);
	void sendToTarget(const QByteArray &data);
};

#endif // CLIENT_H
