#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QDir>
#include <QHostAddress>
#include <vector>

#include "http.h"

struct Host{
	QHostAddress ip;
	uint16_t port = 0;
};

struct UserGrpup{
	enum{
		users,
		admins,
	};
};

struct Control{
	enum{
		VERSION				= 0x01,
		USERS				= 0x02,
		BLACKLIST_ADDRS		= 0x03,
		AUTH				= 0x37,
		INFO				= 0x73,
	};
};

struct User{
	QString login;
	QString pass;
	uint8_t group = UserGrpup::users;
	uint32_t lastLoginTimestamp			= 0;
	uint32_t maxConnections				= 37;
	std::vector<Host> accessList;
	std::vector<Host> blockList;
	uint32_t inBytes					= 0;
	uint32_t outBytes					= 0;
	uint32_t bytesMax					= 536870912;
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
	QByteArray downArrowIMG;
	QByteArray upArrowIMG;
};

struct Config{
	bool verbose						= false;
	uint8_t logLevel					= 3;
#ifdef __linux__
	QString confFile					= "/etc/DrSmyrke/MyProxy/config.ini";
	QString logFile						= "/var/log/socksproxy.log";
	QString accessFile					= "/etc/DrSmyrke/MyProxy/access.list";
	QString usersFile					= "/etc/DrSmyrke/MyProxy/users.list";
#elif _WIN32
	QString confFile					= QDir::homePath() + "/MyProxy/config.ini";
	QString logFile						= QDir::homePath() + "/MyProxy/socksproxy.log";
	QString accessFile					= QDir::homePath() + "/MyProxy/access.list";
	QString usersFile					= QDir::homePath() + "/MyProxy/users.list";
#endif
	uint16_t port						= 7301;
	uint16_t controlPort				= 7302;
	bool settingsSave					= false;
	bool usersSave						= false;
	std::vector<User> users;
	std::map<QString,uint32_t> usersConnections;
	QByteArray realmString				= "ProxyAuth";
	QString version;
	HtmlPage page;
	uint8_t httpAuthMethod				= http::AuthMethod::Digest;
	uint8_t maxFailedAuthorization		= 3;
};

struct BanData{
	QHostAddress addr;
	uint8_t sec;
};

struct AccessList{
	std::vector<QHostAddress> socks4Access;
	QStringList blackDomains;
	std::vector<Host> blackIPs;
	std::vector<Host> blackIPsDynamic;
	QStringList whiteDomains;
	std::vector<Host> whiteIPs;
	QList< BanData > banList;
	bool accessFileSave = false;
};

namespace app {
	extern Config conf;
	extern AccessList accessList;

	void loadSettings();
	void saveSettings();
	bool parsArgs(int argc, char *argv[]);
	void setLog(const uint8_t logLevel, const QString &mess);
	void addGlobalBlackAddr(const QString &str);
	void addGlobalBlackIP(const Host &host);
	void addGlobalBlackIPDynamic(const Host &host);
	void updateBlackWhiteDomains();
	bool isBlockHost(const Host &addr);
	void loadAccessFile();
	void saveAccessFile();
	bool addUser(const QString &login, const QString &pass, const uint8_t group = UserGrpup::users);
	bool findUser(const QString &login);
	bool isSocks4Access(const QHostAddress &addr);
	bool chkAuth(const QString &login, const QString &pass);
	bool chkAuth2(const QString &login, const QString &pass);
	User getUserData(const QString &login);
	bool isBan(const QHostAddress &addr);
	void addBAN(const QHostAddress &addr, const uint8_t timeout = 60);
	bool isBlockedDomName(const QString &domName);
	void loadUsers();
	void saveUsers();
	bool isBlockedToUser(const QString &login, const Host &host);
	void changeUserConnection(const QString &login, const int connCount);
	uint32_t getUserConnectionsNum(const QString &login);
	uint8_t getUserGroupFromName(const QString &name);
	QString getUserGroupNameFromID(const uint8_t id);
	void updateUserLoginTimeStamp(const QString &login);
	void updateBanList();
	uint8_t getTimeBan(const QHostAddress &addr);
	void getIPsFromDomName(const QString &domName, const uint16_t port, std::vector<Host> &data);
	void getIPFromDomName(const QString &domName, Host &host);
	void updateListFromList(const QStringList &list, std::vector<Host> &data);
	void updateListFromList(const std::vector<Host> &data, QStringList &list);
	QString getHtmlPage(const QString &content);
	void addBytesInTraffic(const QString &login, const uint32_t bytes = 0);
	void addBytesOutTraffic(const QString &login, const uint32_t bytes = 0);
	QByteArray processingRequest(const QString &method, const QMap< QByteArray, QByteArray > &args, const User &userData);
	void loadResource(const QString &fileName, QByteArray &data);
	QByteArray getHtmlPage(const QByteArray &title, const QByteArray &content);

	uint8_t getHttpAuthMethodFromString(const QString &string);
	QString getHttpAuthMethodFromCode(const uint8_t code);
	QByteArray getNonceCode();
	QByteArray getHA1Code(const QString &login);
	QString getHttpAuthString();
	bool isMaxConnections(const QString &login);
	QString getUserPass(const QString &login);
	bool isTrafficLimit(const QString &login);
	QString getDateTime(const uint32_t timeStamp);
	bool changePassword(const QString &login, const QString &newPassword);
	bool changeMaxConnections(const QString &login, const uint32_t maxConnections);
	bool changeMaxBytes(const QString &login, const uint32_t bytesMax);
}

#endif // GLOBAL_H
