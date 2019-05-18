#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
	Q_OBJECT
public:
	explicit Client(QObject *parent = nullptr);
	void setSocketDescriptor(qintptr socketDescriptor);
	qintptr getSocketDescriptor() const { return m_pClient->socketDescriptor(); }
	void stop();
signals:
	void signal_finished(qintptr socketDescriptor);
private slots:
	void slot_clientReadyRead();
private:
	QTcpSocket* m_pClient;
	QTcpSocket* m_pTarget;
	uint8_t m_proxyType;
};

#endif // CLIENT_H
