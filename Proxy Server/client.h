#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include "global.h"
#include "http.h"

class Client : public QObject
{
	Q_OBJECT
public:
	explicit Client(QObject *parent = nullptr);
	void setSocketDescriptor(qintptr socketDescriptor);
	qintptr getSocketDescriptor() const { return m_pClient->socketDescriptor(); }
	void stop();
	bool isFinished(){ return m_finished; }
	QString getUserLogin(){ return m_user.login; }
signals:
	void signal_finished();
	void signal_authOK();
private slots:
	void slot_clientReadyRead();
	void slot_targetReadyRead();
	void slot_targetDisconnected();
	void slot_clientDisconnected();
private:
	QTcpSocket* m_pClient;
	QTcpSocket* m_pTarget;
	bool m_tunnel = false;
	bool m_auth = false;
	uint8_t failedAuthorization = 0;
	User m_user;
	uint8_t m_proto = http::Proto::UNKNOW;
	bool m_finished = false;

	void sendResponse(const uint16_t code, const QString &comment);
	void sendNoAuth();
	void sendNoAccess();
	void sendToClient(const QByteArray &data);
	void sendToTarget(const QByteArray &data);
	void parsAuth(const QString &string);
};

#endif // CLIENT_H
