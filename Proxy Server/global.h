#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <vector>

#include "http.h"
#include "myfunctions.h"

struct UserGrpup{
	enum{
		users,
		admins,
	};
};

struct User{
	QString login;
	QString pass;
	uint8_t group = UserGrpup::users;
	uint32_t lastLoginTimestamp = 0;
	uint32_t connections = 0;
	uint32_t maxConnections = 37;
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
#ifdef __linux__
	QString logFile						= "/var/log/webproxy.log";
	QString blackUrlsFile				= "/usr/share/MyProxy/blackUrls.list";
	QString blackAddrsFile				= "/usr/share/MyProxy/blackAddrs.list";
#elif _WIN32
	QString logFile						= QDir::homePath() + "/MyProxy/webproxy.log";
	QString blackUrlsFile				= QDir::homePath() + "/MyProxy/blackUrls.list";
	QString blackAddrsFile				= QDir::homePath() + "/MyProxy/blackAddrs.list";
#endif
	uint8_t maxThreads					= 3;
	uint8_t maxClients					= 37;
	uint16_t port						= 7301;
	std::vector<QString> socksClients;
	std::vector<User> users;
	uint8_t authMethod					= http::AuthMethod::Digest;
	HtmlPage page;
	uint8_t maxFailedAuthorization		= 5;
	uint16_t maxStatusListSize			= 1024;
	bool saveSettings					= false;
	QString adminkaHostAddr				= "config:73";
	QByteArray realmString				= "ProxyAuth";
};

struct BlackList{
	std::vector<QString> urls;
	std::vector<QString> addrs;
	bool urlsFileSave = false;
	bool addrsFileSave = false;
};

struct Status{
	std::vector<uint8_t> threads;
	std::vector<QString> urls;
	std::vector<QString> addrs;
};

namespace app {
	extern Config conf;
	extern Status state;
	extern BlackList blackList;


	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	QByteArray getHtmlPage(const QByteArray &title, const QByteArray &content);
	bool addUser(const QString &login, const QString &pass, const uint8_t group = UserGrpup::users);
	bool chkAuth(const QString &login, const QString &pass);
	void usersConnectionsClear();
	void usersConnectionsNumAdd(const QString &login, const int32_t num = 1);
	void addOpenUrl(const QUrl &url);
	void addOpenAddr(const QString &addr);
	void loadResource(const QString &fileName, QByteArray &data);
	void loadBlackList(const QString &fileName, std::vector<QString> &data);
	void saveBlackList(const QString &fileName, const std::vector<QString> &data);
	void addGlobalBlackUrl(const QString &str);
	void addGlobalBlackAddr(const QString &str);
	QByteArray parsRequest(const QString &data , const User &userData);
	void getUserData(User &userData, const QString &login);
	QByteArray processingRequest(const QString &method, const std::map<QByteArray, std::vector<QByteArray> > &args, const User &userData);
	bool findInBlackList(const QString &url, const std::vector<QString> &data);
	QString getAuthString();
	QByteArray getNonceCode();
	QByteArray getHA1Code(const QString &login);
	void updateLoginTimestamp(const QString &login);
}

#endif // GLOBAL_H
