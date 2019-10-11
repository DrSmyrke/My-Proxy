#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

#include "global.h"
#include "myfunctions.h"
#include "http.h"

class Client : public QObject
{
	Q_OBJECT
public:
	struct Proto{
		enum Version{
			UNKNOWN,
			MY			= 0x73,
			AUTH		= 0x01,
			HTTP,
			HTTPS,
			SOCKS4		= 0x04,
			SOCKS5		= 0x05,

		};
	};
	explicit Client(qintptr descriptor, QObject *parent = nullptr);
	void run();
signals:
	void signal_finished();
public slots:
	void slot_stop();
private slots:
	void slot_clientReadyRead();
	void slot_targetReadyRead();
private:
	QTcpSocket* m_pClient;
	QTcpSocket* m_pTarget;
	bool m_tunnel;
	bool m_auth;
	QString m_userLogin;
	QString m_userPass;
	QString m_targetHostStr;
	uint16_t m_targetHostPort;
	uint8_t m_proto;
	uint8_t m_failedAuthorization;

	void sendResponse(const uint16_t code, const QString &comment);
	void sendRawResponse(const uint16_t code, const QString &comment, const QString &data, const QString &mimeType);
	void sendToClient(const QByteArray &data);
	void sendToTarget(const QByteArray &data);
	void parsHttpProxy(http::pkt &pkt, const int32_t sizeInData);
	void parsAuth(const QString &string, const QString &method);
	void sendNoAuth();
	void sendNoAccess();
	void moveToLockedPage(const QString &reffer);
	bool isSocksRequest(const QByteArray &buff);
	void socksPktProcessing(QByteArray &buff);
};

#endif // CLIENT_H
