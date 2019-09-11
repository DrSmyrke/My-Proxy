#include "global.h"
#include "myfunctions.h"

#include <QDateTime>
#include <QSettings>
#include <QHostInfo>

namespace app {
	Config conf;
	BlackList blackList;
	AccessLists accessLists;

	void loadSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		app::setLog( 3, QString("LOAD SETTINGS [%1]...").arg(app::conf.confFile) );

		QSettings settings(app::conf.confFile, QSettings::IniFormat);

		app::conf.port = settings.value("SOCKS_PROXY/port",app::conf.port).toUInt();
		app::conf.blackAddrsFile = settings.value("SOCKS_PROXY/blackAddrsFile",app::conf.blackAddrsFile).toString();
		app::conf.socks4AccessFile = settings.value("SOCKS_PROXY/socks4AccessFile",app::conf.socks4AccessFile).toString();
		app::conf.usersFile = settings.value("SOCKS_PROXY/usersFile",app::conf.usersFile).toString();
		app::conf.logFile = settings.value("SOCKS_PROXY/logFile",app::conf.logFile).toString();
		app::conf.logLevel = settings.value("SOCKS_PROXY/logLevel",app::conf.logLevel).toUInt();
		app::conf.verbose = settings.value("SOCKS_PROXY/verbose",app::conf.verbose).toBool();

#ifdef __linux__

#elif _WIN32
	QDir dir(QDir::homePath());
	dir.mkdir("MyProxy");
#endif

		app::loadBlackList( app::conf.blackAddrsFile, app::blackList.nameAddrs );

		app::loadAccessFile( app::conf.socks4AccessFile, app::accessLists.socks4access );

		app::updateBlackIPAddrs();

		app::loadUsers();

		if( settings.allKeys().size() == 0 ){
			app::conf.saveSettings = true;
			app::saveSettings();
		}
	}

	void saveSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		if( app::conf.saveSettings ){
			app::setLog( 3, QString("SAVE SETTINGS [%1]...").arg(app::conf.confFile) );
			QSettings settings(app::conf.confFile, QSettings::IniFormat);
			settings.clear();
			settings.setValue("SOCKS_PROXY/port",app::conf.port);
			settings.setValue("SOCKS_PROXY/blackAddrsFile",app::conf.blackAddrsFile);
			settings.setValue("SOCKS_PROXY/socks4AccessFile",app::conf.socks4AccessFile);
			settings.setValue("SOCKS_PROXY/usersFile",app::conf.usersFile);
			settings.setValue("SOCKS_PROXY/logFile",app::conf.logFile);
			settings.setValue("SOCKS_PROXY/logLevel",app::conf.logLevel);
			settings.setValue("SOCKS_PROXY/verbose",app::conf.verbose);

			app::conf.saveSettings = false;
		}
		if( app::blackList.addrsFileSave ){
			app::saveBlackList( app::conf.blackAddrsFile, app::blackList.nameAddrs );
			app::blackList.addrsFileSave = false;
		}

		if( app::accessLists.socks4AccessFileFileSave ){
			app::saveAccessFile( app::conf.socks4AccessFile, app::accessLists.socks4access );
			app::accessLists.socks4AccessFileFileSave = false;
		}

		if( app::conf.saveUsers ){
			app::saveUsers();
			app::conf.saveUsers = false;
		}
	}

	bool parsArgs(int argc, char *argv[])
	{
		bool ret = true;
		for(int i=0;i<argc;i++){
			if(QString(argv[i]).indexOf("-")==0){
				if(QString(argv[i]) == "--help" or QString(argv[1]) == "-h"){
					printf("Usage: %s [OPTIONS]\n"
							"  -l <FILE>    log file\n"
							"  -v    Verbose output\n"
							"\n", argv[0]);
					ret = false;
				}
				if(QString(argv[i]) == "-l") app::conf.logFile = QString(argv[++i]);
				if(QString(argv[i]) == "-v") app::conf.verbose = true;
			//}else{
			//	bool ok = false;
			//	QString(argv[i]).toInt(&ok,10);
			//	if(ok) app::conf.port = QString(argv[i]).toInt();
			}
		}
		return ret;
	}

	void setLog(const uint8_t logLevel, const QString &mess)
	{
		if(app::conf.logLevel < logLevel or app::conf.logLevel == 0) return;

		QDateTime dt = QDateTime::currentDateTime();
		QString str = dt.toString("yyyy.MM.dd [hh:mm:ss] ") + mess + "\n";

		if( app::conf.verbose ){
			printf( "%s", str.toUtf8().data() );
			fflush( stdout );
		}

		if( app::conf.logFile.isEmpty() ) return;
		FILE* f;
		f = fopen(app::conf.logFile.toUtf8().data(),"a+");
		fwrite(str.toUtf8().data(),static_cast<size_t>(str.length()),1,f);
		fclose(f);
	}

	void loadBlackList(const QString &fileName, std::vector<QString> &data)
	{
		app::setLog( 3, "LOAD BLACK ADDRS LIST..." );
		data.clear();
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			QByteArray str;
			char sym;
			while(!file.atEnd()){
				file.read( &sym, 1 );
				if( sym == '\n' ){
					data.push_back( str );
					str.clear();
					continue;
				}
				str.append( sym );
			}
			file.close();
		}
	}

	void saveBlackList(const QString &fileName, const std::vector<QString> &data)
	{
		app::setLog( 3, "SAVE BLACK ADDRS LIST..." );
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
			for( auto elem:data ){
				file.write( elem.toLatin1() );
				file.write( "\n" );
			}
			file.close();
		}
	}

	void addGlobalBlackAddr(const QString &str)
	{
		app::setLog( 4, "SAVE BLACK ADDRS LIST..." );
		bool find = false;

		for( auto elem:app::blackList.nameAddrs ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::blackList.nameAddrs.push_back( str );
			app::blackList.addrsFileSave = true;
		}
	}

	void updateBlackIPAddrs()
	{
		app::setLog( 3, "UPDATE BLACK IP LIST..." );

		app::blackList.ipAddrs.clear();

		for( auto addr:app::blackList.nameAddrs ){
			auto info = QHostInfo::fromName( addr );
			app::setLog( 4, QString("GET INFO for [%1]").arg(addr) );
			if( info.error() == QHostInfo::NoError ){
				for(auto elem:info.addresses()){
					app::blackList.ipAddrs.push_back( elem );
					app::setLog( 5, QString("SET IP [%1]").arg(elem.toString()) );
				}
			}
		}
	}

	bool isBlockAddr(const QHostAddress& addr)
	{
		bool res = false;

		for( auto elem:app::blackList.ipAddrs ){
			if( elem == addr ){
				res = true;
				break;
			}
		}

		return res;
	}

	void loadAccessFile(const QString &fileName, std::vector<QHostAddress> &data)
	{
		app::setLog( 3, QString("LOAD ACCESS LIST... [%1]").arg(fileName) );

		data.clear();

		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			QByteArray str;
			char sym;
			while(!file.atEnd()){
				file.read( &sym, 1 );
				if( sym == '\n' ){
					QHostAddress addr;
					if( addr.setAddress( QString(str) ) ) data.push_back( addr );
					str.clear();
					continue;
				}
				str.append( sym );
			}
			file.close();
		}
	}

	void saveAccessFile(const QString &fileName, std::vector<QHostAddress> &data)
	{
		app::setLog( 3, QString("SAVE ACCESS LIST... [%1]").arg(fileName) );
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
			for( auto elem:data ){
				file.write( elem.toString().toUtf8() );
				file.write( "\n" );
			}
			file.close();
		}
	}

	bool addUser(const QString &login, const QString &pass, const uint8_t group)
	{
		bool res = false;

		if( !findUser( login ) ){
			User user;
			user.login = login;
			user.pass = mf::md5( app::conf.realmString + ":->" + pass.toUtf8() ).toHex();
			user.group = group;
			app::conf.users.push_back( user );
			app::conf.saveUsers = true;
			res = true;
		}

		return res;
	}

	bool findUser(const QString &login)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				res = true;
				break;
			}
		}

		return res;
	}

	bool isSocks4Access(const QHostAddress &addr)
	{
		bool res = false;

		for( auto accessAddr:app::accessLists.socks4access ){
			if( addr == accessAddr ){
				res = true;
				break;
			}
		}

		return res;
	}

	bool chkAuth(const QString &login, const QString &pass)
	{
		bool res = false;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				auto newPass = mf::md5( app::conf.realmString + ":->" + pass.toUtf8() ).toHex();
				if( newPass == user.pass ) res = true;
				break;
			}
		}

		return res;
	}

	User getUserData(const QString &login)
	{
		User data;

		for( auto user:app::conf.users ){
			if( login == user.login ){
				data = user;
				break;
			}
		}

		return data;
	}

	bool isBan(const QHostAddress &addr)
	{
		bool res = false;

		while( app::blackList.BANipAddrsLock );
		app::blackList.BANipAddrsLock = true;
		for( auto elem:app::blackList.BANipAddrs ){
			if( addr == elem.first ){
				res = true;
				break;
			}
		}
		app::blackList.BANipAddrsLock = false;

		return res;
	}

	void addBAN(const QHostAddress &addr)
	{
		uint8_t timeout = 30;

		while( app::blackList.BANipAddrsLock );
		if( !isBan( addr ) ){

			std::pair<QHostAddress,uint32_t> data;
			data.first = addr;
			data.second = timeout;

			app::blackList.BANipAddrs.push_back( data );
			return;
		}

		app::blackList.BANipAddrsLock = true;

		for( auto &elem:app::blackList.BANipAddrs ){
			if( addr == elem.first ){
				elem.second += 30;
				break;
			}
		}
		app::blackList.BANipAddrsLock = false;
	}

	bool isBlockedDomName(const QString &domName)
	{
		bool res = false;

		for( auto elem:app::blackList.nameAddrs ){
			if( mf::strFind( elem, domName ) ){
				res = true;
				break;
			}
		}

		return res;
	}

	void loadUsers()
	{
		if( app::conf.usersFile.isEmpty() ){
			app::setLog( 3, QString("LOAD USER FILE [%1] ... ERROR not defined").arg( app::conf.usersFile ) );
			return;
		}

		if( !mf::checkFile( app::conf.usersFile.toUtf8().data() ) ){
			app::setLog( 3, QString("LOAD USER FILE [%1] ... ERROR not found").arg( app::conf.usersFile ) );
			return;
		}

		app::setLog( 4, QString("LOAD USER FILE [%1] ...").arg( app::conf.usersFile ) );

		app::conf.users.clear();

		QSettings users( app::conf.usersFile, QSettings::IniFormat );

		for( auto group:users.childGroups() ){
			app::setLog( 4, QString("   FOUND USER [%1]").arg( group ) );
			users.beginGroup( group );

			User user;

			user.login				= group;
			user.group				= app::getUserGroupFromName( users.value( "group", "" ).toString() );
			user.pass				= users.value( "password", "" ).toString();
			user.maxConnections		= users.value( "maxConnections", 10 ).toUInt();
			user.accessList			= users.value( "accessList", "*" ).toString().split(",");
			user.blockList			= users.value( "blockList", "*" ).toString().split(",");

			app::setLog( 5, QString("      USER PARAM [%1][%2][%3]").arg( user.group ).arg( user.pass ).arg( user.maxConnections ) );

			app::conf.users.push_back( user );

			users.endGroup();
		}
	}

	void saveUsers()
	{
		if( app::conf.usersFile.isEmpty() ){
			app::setLog( 3, QString("SAVE USER FILE [%1] ... ERROR not defined").arg( app::conf.usersFile ) );
			return;
		}

		app::setLog( 4, QString("SAVE USER FILE [%1] ...").arg( app::conf.usersFile ) );

		QSettings users( app::conf.usersFile, QSettings::IniFormat );
		users.clear();

		for( auto user:app::conf.users ){
			users.beginGroup( user.login );

			users.setValue( "group", app::getUserGroupNameFromID( user.group ) );
			users.setValue( "password", user.pass );
			users.setValue( "maxConnections", user.maxConnections );
			users.setValue( "accessList", user.accessList.join(",") );
			users.setValue( "blockList", user.blockList.join(",") );

			users.endGroup();
		}
	}

	bool isBlockedUserList(const QString &login, const QString &addr)
	{
		bool res = false;

		for( auto elem:app::getUserData( login ).blockList ){
			if( mf::strFind( elem, addr ) ){
				res = true;
				break;
			}
		}

		return res;
	}

	bool isAccessUserList(const QString &login, const QString &addr)
	{
		bool res = false;

		for( auto elem:app::getUserData( login ).accessList ){
			if( mf::strFind( elem, addr ) ){
				res = true;
				break;
			}
		}

		return res;
	}

	void changeUserConnection(const QString &login, const int connCount)
	{
		if( connCount == 0 ){
			app::conf.usersConnections[login] = 0;
		}else{

			if( connCount < 0 ){
				uint32_t r = 0 - connCount;
				if( app::conf.usersConnections[login] < r ){
					app::conf.usersConnections[login] = 0;
				}else{
					app::conf.usersConnections[login] += connCount;
				}
			}else{
				app::conf.usersConnections[login] += connCount;
			}
		}
	}

	uint32_t getUserConnectionsNum(const QString &login)
	{
		uint32_t num = 0;

		auto iter = app::conf.usersConnections.find( login );
		if( iter != app::conf.usersConnections.end() ) num = (*iter).second;

		return num;
	}

	uint8_t getUserGroupFromName(const QString &name)
	{
		uint8_t res = UserGrpup::users;

		if( name == "admin" ){
			res = UserGrpup::admins;
			return res;
		}

		return res;
	}

	QString getUserGroupNameFromID(const uint8_t id)
	{
		QString res = "user";

		switch( id ){
			case UserGrpup::admins:		res = "admin";	break;
		}

		return res;
	}

}
