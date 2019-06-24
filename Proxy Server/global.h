#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <vector>
#include <QSqlDatabase>

#include "http.h"
#include "myfunctions.h"

struct UserGrpups{
	enum{
		users,
		admins,
	};
};

struct User{
	QString login;
	QString pass;
	uint8_t group = UserGrpups::users;
	uint32_t lastLoginTimestamp = 0;
	uint32_t connections = 0;
};

struct HtmlPage{
	QByteArray top;
	QByteArray bottom;
	QByteArray menu;
	QByteArray indexJS;
	QByteArray colorCSS;
	QByteArray buttonsCSS;
	QByteArray indexCSS;
	QByteArray admin;
	QByteArray state;
	QByteArray config;
	QByteArray index;
};

struct Config{
	bool verbose						= false;
	uint8_t logLevel					= 3;
	QString logFile						= "webProxy.log";
	QString baseFile					= "webProxy.sqlite";
	uint8_t maxThreads					= 3;
	uint8_t maxClients					= 37;
	uint16_t port						= 7300;
	std::vector<QString> socksClients;
	std::vector<User> users;
	uint8_t authMetod					= http::AuthMethod::Basic;
	QString codeWord					= "DeadBeef";
	HtmlPage page;
	uint8_t maxFailedAuthorization		= 5;
	uint16_t maxStatusListSize			= 65000;
};

struct Status{
	std::vector<uint8_t> threads;
	std::vector<QString> urls;
	std::vector<QString> addrs;
};

namespace app {
	extern Config conf;
	extern Status state;
	extern QSqlDatabase sdb;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	QByteArray getHtmlPage(const QByteArray &title, const QByteArray &content);
	bool addUser(const QString &login, const QString &pass);
	bool passIsValid(const QString &pass, const QString &hash);
	bool chkAuth(const QString &login, const QString &pass);
	void usersConnectionsClear();
	void usersConnectionsNumAdd(const QString &login, const uint32_t num);
	void reloadUsers();
	void saveDataToBase();
	void addOpenUrl(const QUrl &url);
	void addOpenAddr(const QString &addr);
	void loadResource(const QString &fileName, QByteArray &data);
	void addBlackUrl(const QString &str);
	QByteArray parsRequest(const QString &data );
}

#endif // GLOBAL_H
