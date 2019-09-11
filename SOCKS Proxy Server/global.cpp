#include "global.h"
#include "myfunctions.h"

#include <QDateTime>
#include <QSettings>
#include <QHostInfo>

namespace app {
	Config conf;
	AccessList accessList;
	LockFlags lockFlags;

	void loadSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		app::setLog( 3, QString("LOAD SETTINGS [%1]...").arg(app::conf.confFile) );

		QSettings settings(app::conf.confFile, QSettings::IniFormat);

		app::conf.port = settings.value("SOCKS_PROXY/port",app::conf.port).toUInt();
		app::conf.accessFile = settings.value("SOCKS_PROXY/accessFile",app::conf.accessFile).toString();
		app::conf.usersFile = settings.value("SOCKS_PROXY/usersFile",app::conf.usersFile).toString();
		app::conf.logFile = settings.value("SOCKS_PROXY/logFile",app::conf.logFile).toString();
		app::conf.logLevel = settings.value("SOCKS_PROXY/logLevel",app::conf.logLevel).toUInt();
		app::conf.verbose = settings.value("SOCKS_PROXY/verbose",app::conf.verbose).toBool();

#ifdef __linux__

#elif _WIN32
	QDir dir(QDir::homePath());
	dir.mkdir("MyProxy");
#endif

		app::loadAccessFile();

		app::updateBlackIPAddrs();

		app::loadUsers();

		if( settings.allKeys().size() == 0 ) app::conf.settingsSave = true;
	}

	void saveSettings()
	{
		if( app::conf.confFile.isEmpty() ) return;

		if( app::conf.settingsSave ){
			app::setLog( 3, QString("SAVE SETTINGS [%1]...").arg(app::conf.confFile) );
			QSettings settings(app::conf.confFile, QSettings::IniFormat);
			settings.clear();
			settings.setValue("SOCKS_PROXY/port",app::conf.port);
			settings.setValue("SOCKS_PROXY/accessFile",app::conf.accessFile);
			settings.setValue("SOCKS_PROXY/usersFile",app::conf.usersFile);
			settings.setValue("SOCKS_PROXY/logFile",app::conf.logFile);
			settings.setValue("SOCKS_PROXY/logLevel",app::conf.logLevel);
			settings.setValue("SOCKS_PROXY/verbose",app::conf.verbose);

			app::conf.settingsSave = false;
		}

		if( app::accessList.accessFileSave ) app::saveAccessFile();
		if( app::conf.usersSave ) app::saveUsers();
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
		QFile f;
		f.setFileName( app::conf.logFile );
		if( f.open( QIODevice::Append ) ){
			f.write( str.toUtf8() );
			f.close();
		}
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
		bool find = false;

		while( app::lockFlags.blackDomains );
		app::lockFlags.blackDomains = true;

		for( auto elem:app::accessList.blackDomains ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::accessList.blackDomains.push_back( str );
			app::accessList.accessFileSave = true;
		}

		app::lockFlags.blackDomains = false;
	}

	void addGlobalBlackIP(const QHostAddress &addr)
	{
		bool find = false;

		while( app::lockFlags.blackIPs );
		app::lockFlags.blackIPs = true;

		for( auto elem:app::accessList.blackIPs ){
			if( elem == addr ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::accessList.blackIPs.push_back( addr );
			app::accessList.accessFileSave = true;
		}

		app::lockFlags.blackIPs = false;
	}

	void updateBlackIPAddrs()
	{
		app::setLog( 3, "UPDATE BLACK IP LIST..." );

		while( app::lockFlags.blackDomains );
		app::lockFlags.blackDomains = true;

		for( auto addr:app::accessList.blackDomains ){
			auto info = QHostInfo::fromName( addr );
			app::setLog( 4, QString("GET INFO for [%1]").arg(addr) );
			if( info.error() == QHostInfo::NoError ){
				for(auto elem:info.addresses()){
					app::addGlobalBlackIP( elem );
					app::setLog( 5, QString("SET IP [%1]").arg(elem.toString()) );
				}
			}
		}

		app::lockFlags.blackDomains = false;
	}

	bool isBlockAddr(const QHostAddress& addr)
	{
		bool res = false;

		while( app::lockFlags.blackIPs );

		for( auto elem:app::accessList.blackIPs ){
			if( elem == addr ){
				res = true;
				break;
			}
		}

		return res;
	}

	void loadAccessFile()
	{
		if( app::conf.accessFile.isEmpty() ){
			app::setLog( 3, QString("LOAD ACCESS FILE [%1] ... ERROR not defined").arg( app::conf.usersFile ) );
			return;
		}

		if( !mf::checkFile( app::conf.accessFile.toUtf8().data() ) ){
			app::setLog( 3, QString("LOAD ACCESS FILE [%1] ... ERROR not found").arg( app::conf.usersFile ) );
			return;
		}

		app::setLog( 3, QString("LOAD ACCESS FILE... [%1]").arg( app::conf.usersFile ) );

		app::accessList.BANipAddrs.clear();
		app::accessList.blackDomains.clear();
		app::accessList.blackIPs.clear();
		app::accessList.socks4Access.clear();
		app::accessList.whiteDomains.clear();
		app::accessList.whiteIPs.clear();

		QSettings fileData( app::conf.accessFile, QSettings::IniFormat );

		for( auto group:fileData.childGroups() ){
			app::setLog( 4, QString("   FOUND GROUP [%1]").arg( group ) );
			fileData.beginGroup( group );

			qDebug()<<fileData.childKeys();

			fileData.endGroup();
		}


//		for( auto group:users.childGroups() ){
//			app::setLog( 4, QString("   FOUND USER [%1]").arg( group ) );
//			users.beginGroup( group );

//			User user;

//			user.login				= group;
//			user.group				= app::getUserGroupFromName( users.value( "group", "" ).toString() );
//			user.pass				= users.value( "password", "" ).toString();
//			user.maxConnections		= users.value( "maxConnections", 10 ).toUInt();
//			user.accessList			= users.value( "accessList", "*" ).toString().split(",");
//			user.blockList			= users.value( "blockList", "*" ).toString().split(",");

//			app::setLog( 5, QString("      USER PARAM [%1][%2][%3]").arg( user.group ).arg( user.pass ).arg( user.maxConnections ) );

//			app::conf.users.push_back( user );

//			users.endGroup();
//		}
	}

	void saveAccessFile()
	{
		if( app::conf.accessFile.isEmpty() ){
			app::setLog( 3, QString("SAVE ACCESS FILE [%1] ... ERROR not defined").arg( app::conf.accessFile ) );
			return;
		}

		app::setLog( 4, QString("SAVE ACCESS FILE [%1] ...").arg( app::conf.accessFile ) );

		QSettings accessFile( app::conf.accessFile, QSettings::IniFormat );
		accessFile.clear();

		accessFile.beginGroup( "Socks4Access" );
		for( auto elem:app::accessList.socks4Access ) accessFile.setValue( elem.toString(), QVariant() );
		accessFile.endGroup();

		accessFile.beginGroup( "WhiteDomains" );
		for( auto elem:app::accessList.whiteDomains ) accessFile.setValue( elem, QVariant() );
		accessFile.endGroup();

		accessFile.beginGroup( "WhiteAddrs" );
		for( auto elem:app::accessList.whiteIPs ) accessFile.setValue( elem.toString(), QVariant() );
		accessFile.endGroup();

		accessFile.beginGroup( "BlackDomains" );
		for( auto elem:app::accessList.blackDomains ) accessFile.setValue( elem, QVariant() );
		accessFile.endGroup();

		accessFile.beginGroup( "BlackAddrs" );
		for( auto elem:app::accessList.blackIPs ) accessFile.setValue( elem.toString(), QVariant() );
		accessFile.endGroup();

		accessFile.beginGroup( "BanIPs" );
		for( auto elem:app::accessList.BANipAddrs ) accessFile.setValue( elem.first.toString(), elem.second );
		accessFile.endGroup();

		app::accessList.accessFileSave = false;
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
			app::conf.usersSave = true;
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

		for( auto accessAddr:app::accessList.socks4Access ){
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

		while( app::lockFlags.BANipAddrs );
		app::lockFlags.BANipAddrs = true;

		for( auto elem:app::accessList.BANipAddrs ){
			if( addr == elem.first ){
				res = true;
				break;
			}
		}
		app::lockFlags.BANipAddrs = false;

		return res;
	}

	void addBAN(const QHostAddress &addr)
	{
		uint8_t timeout = 30;

		while( app::lockFlags.BANipAddrs );

		if( !isBan( addr ) ){

			std::pair<QHostAddress,uint32_t> data;
			data.first = addr;
			data.second = timeout;

			app::accessList.BANipAddrs.push_back( data );
			return;
		}

		app::lockFlags.BANipAddrs = true;

		for( auto &elem:app::accessList.BANipAddrs ){
			if( addr == elem.first ){
				elem.second += 30;
				break;
			}
		}
		app::lockFlags.BANipAddrs = false;
	}

	bool isBlockedDomName(const QString &domName)
	{
		bool res = false;

		for( auto elem:app::accessList.blackDomains ){
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

		app::conf.usersSave = false;
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
