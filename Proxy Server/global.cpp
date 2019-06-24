#include "global.h"

#include <QDateTime>
#include <QSettings>
//TODO: remove qdebug
#include <QDebug>
#include <QSql>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlError>
#include <QSqlRecord>
#include <QUrl>

namespace app {
	Config conf;
	Status state;
	QSqlDatabase sdb;

	void loadSettings()
	{
		QSettings settings("MySoft","WebProxy");

		app::conf.maxThreads = settings.value("SERVER/maxThreads",app::conf.maxThreads).toUInt();
		app::conf.port = settings.value("SERVER/port",app::conf.port).toUInt();
		app::conf.baseFile = settings.value("SERVER/baseFile",app::conf.baseFile).toString();

		app::loadResource( ":/pages/assets/top.html", app::conf.page.top );
		app::loadResource( ":/pages/assets/bottom.html", app::conf.page.bottom );
		app::loadResource( ":/pages/assets/menu.html", app::conf.page.menu );
		app::loadResource( ":/pages/assets/index.js", app::conf.page.indexJS );
		app::loadResource( ":/pages/assets/color.css", app::conf.page.colorCSS );
		app::loadResource( ":/pages/assets/index.css", app::conf.page.indexCSS );
		app::loadResource( ":/pages/assets/buttons.css", app::conf.page.buttonsCSS );
		app::loadResource( ":/pages/assets/admin.html", app::conf.page.admin );
		app::loadResource( ":/pages/assets/state.html", app::conf.page.state );
		app::loadResource( ":/pages/assets/config.html", app::conf.page.config );
		app::loadResource( ":/pages/assets/index.html", app::conf.page.index );

		if( !app::conf.baseFile.isEmpty() ){
			if( app::sdb.isOpen() ) app::sdb.close();
			app::sdb = QSqlDatabase::addDatabase("QSQLITE");
			app::sdb.setDatabaseName( app::conf.baseFile );
			if( !app::sdb.open() ) app::setLog(1,QString("app::loadSettings cannot open baseFile [%1]").arg(app::conf.baseFile));
			if( app::sdb.isOpen() ){
				QSqlQuery a_query;
				if (!a_query.exec("SELECT * FROM users")){
					bool res = a_query.exec("CREATE TABLE `users` ( `login`	TEXT NOT NULL UNIQUE, `password`	TEXT NOT NULL, `group`	INTEGER NOT NULL DEFAULT 0, `lastLoginTimestamp`	INTEGER NOT NULL DEFAULT 0, `connections`	INTEGER NOT NULL DEFAULT 0, PRIMARY KEY(`login`) );");
					if( !res ) app::setLog(1,QString("app::loadSettings cannot create user table"));
				}
				if (!a_query.exec("SELECT * FROM blackUrls")){
					bool res = a_query.exec("CREATE TABLE `blackUrls` ( `url`	TEXT NOT NULL UNIQUE );");
					if( !res ) app::setLog(1,QString("app::loadSettings cannot create blackUrls table"));
					if( res ){
						app::addBlackUrl("*/ad");
						app::addBlackUrl("ad??.*");
						app::addBlackUrl("ad?.*");
						app::addBlackUrl("ad.*");
						app::addBlackUrl("*adframe*");
						app::addBlackUrl("*/ad-handler");
						app::addBlackUrl("*/ads");
						app::addBlackUrl("ads???.*");
						app::addBlackUrl("ads.*");
						app::addBlackUrl("adserv.*");
						app::addBlackUrl("*/banner");
						app::addBlackUrl("*/popup");
						app::addBlackUrl("*/popups");
					}
				}
			}
		}
	}

	void saveSettings()
	{
		QSettings settings("MySoft","WebProxy");
		settings.clear();

		settings.setValue("SERVER/maxThreads",app::conf.maxThreads);
		settings.setValue("SERVER/port",app::conf.port);
		settings.setValue("SERVER/baseFile",app::conf.baseFile);
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

	QByteArray getHtmlPage(const QByteArray &title, const QByteArray &content)
	{
		QByteArray ba;
			ba.append( app::conf.page.top );
			ba.append( "		<title>-= " );
			ba.append( title );
			ba.append( " =-</title>\n	</head>\n<body>\n" );
			ba.append( app::conf.page.menu );
			ba.append( content );
			ba.append( app::conf.page.bottom );
		return ba;
	}

	bool addUser(const QString &login, const QString &pass)
	{
		bool res = true;
		auto hash = mf::md5(pass.toLatin1());
		hash.append( app::conf.codeWord );
		User user;
		user.login = login;
		user.pass = mf::md5( hash );
		app::conf.users.push_back( user );

		if( app::sdb.isOpen() ){
			QSqlQuery a_query;
			a_query.exec("SELECT `login` FROM users WHERE `login` = '" + user.login + "'");
			uint32_t count = 0;
			while( a_query.next() ) count++;
			if( !count ){
				a_query.prepare("INSERT INTO users (login, password) VALUES (:login, :password);");
				a_query.bindValue(":login", user.login);
				a_query.bindValue(":password", user.pass);
				if (!a_query.exec()) app::setLog(1,QString("app::addUser cannot add user"));
			}
		}

		return res;
	}

	bool passIsValid(const QString &pass, const QString &hash)
	{
		bool res = false;
		auto newhash = mf::md5(pass.toLatin1());
		newhash.append( app::conf.codeWord );
		res = ( hash == mf::md5( newhash ) )?true:false;
		return res;
	}

	bool chkAuth(const QString &login, const QString &pass)
	{
		bool res = false;
		for( auto &user:app::conf.users){
			if( user.login == login ){
				if( app::passIsValid( pass, user.pass ) ){
					QDateTime dt = QDateTime::currentDateTime();
					user.lastLoginTimestamp = dt.toTime_t();
					res = true;
				}
				break;
			}
		}
		return res;
	}

	void usersConnectionsClear()
	{
		for( auto &user:app::conf.users ) user.connections = 0;
	}

	void usersConnectionsNumAdd(const QString &login, const uint32_t num)
	{
		for( auto &user:app::conf.users ){
			if( user.login == login ){
				user.connections += num;
				break;
			}
		}
	}

	void reloadUsers()
	{
		app::conf.users.clear();
		QSqlQuery a_query;
		User user;

		a_query.exec("SELECT * FROM users");
		QSqlRecord rec = a_query.record();

		while( a_query.next() ){
			user.login = a_query.value(rec.indexOf("login")).toString();
			user.pass = a_query.value(rec.indexOf("password")).toString();
			user.group = a_query.value(rec.indexOf("group")).toInt();
			user.lastLoginTimestamp = a_query.value(rec.indexOf("lastLoginTimestamp")).toInt();
			user.connections = a_query.value(rec.indexOf("connections")).toInt();
			app::conf.users.push_back( user );
		}
	}

	void saveDataToBase()
	{
		app::setLog( 4, QString("saveDataToBase...") );
		if( !app::sdb.isOpen() ) return;


		QSqlQuery a_query;
		a_query.prepare("INSERT INTO users (login, password, group, lastLoginTimestamp, connections)"
									  "VALUES (:login, :password, :group, :lastLoginTimestamp, :connections);");
		for( auto user:app::conf.users ){

		}

		a_query.bindValue(":login", "14");
		a_query.bindValue(":password", "hello world str.");
		a_query.bindValue(":group", "37");
		a_query.bindValue(":lastLoginTimestamp", "37");
		a_query.bindValue(":connections", "37");


		app::setLog( 4, QString("saveDataToBase... complete") );
	}
/*
	QString getStatePage()
	{
		QString str;
		QDateTime dt = QDateTime::currentDateTime();

		str += "<fieldset class=\"block\"><legend>Threads</legend><table>";
		for( uint8_t i = 0; i < app::state.threads.size(); i++ ){
			str += "<tr><td>Thread #" + QString::number( i ) + ":</td><td>" + QString::number( app::state.threads.at( i ) ) + "</td></tr>";
		}
		str += "</table></fieldset>";


		str += "<fieldset class=\"block\"><legend>Active users</legend><table>";
		for( auto user:app::conf.users ){
			uint32_t lastLoginSec = dt.toTime_t() - user.lastLoginTimestamp;
			str += "<tr><td>" + user.login + "</td><td> " + QString::number( lastLoginSec ) + " sec. ago</td></tr>";
		}
		str += "</table></fieldset>";


		str += "<fieldset class=\"block\"><legend>Connections</legend><table>";
		for( auto user:app::conf.users ){
			str += "<tr><td>" + user.login + "</td><td> " + QString::number( user.connections ) + "</td></tr>";
		}
		str += "</table></fieldset>";

		str += "<fieldset class=\"block\"><legend>Open URL`s</legend><table>";
		for( auto url:app::state.urls ){
			str += "<tr><td>" + url + "</td><td></td></tr>";
		}
		str += "</table></fieldset>";

		str += "<fieldset class=\"block\"><legend>Open ADDR`s</legend><table>";
		for( auto addr:app::state.addrs ){
			str += "<tr><td>" + addr + "</td><td></td></tr>";
		}
		str += "</table></fieldset>";

		return str;
	}

	QString getAdminPage()
	{
		QString str;

		str += "<fieldset class=\"block\"><legend>Global blocked url`s</legend><table>";
		//for( auto user:app::conf.users ){
		//	str += "<tr><td>" + user.login + "</td><td> " + QString::number( user.connections ) + "</td></tr>";
		//}
		str += "</table></fieldset>";

		return str;
	}

	QString getConfigPage()
	{

	}

	QString getHomePage()
	{

	}
*/
	void addOpenUrl(const QUrl &url)
	{
		bool find = false;
		for( auto elem:app::state.urls ){
			if( elem == url.toString() ){
				find = true;
				break;
			}
		}
		if( !find ) app::state.urls.push_back( url.toString() );
	}

	void addOpenAddr(const QString &addr)
	{
		bool find = false;
		for( auto elem:app::state.addrs ){
			if( elem == addr ){
				find = true;
				break;
			}
		}
		if( !find ) app::state.addrs.push_back( addr );
	}

	void loadResource(const QString &fileName, QByteArray &data)
	{
		QFile file;

		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			while (!file.atEnd()) data.append( file.readAll() );
			file.close();
		}
	}

	void addBlackUrl(const QString &str)
	{
		if( !app::sdb.isOpen() ) return;

		QSqlQuery a_query;
		a_query.exec("SELECT `url` FROM blackUrls WHERE `url` = '" + str + "'");
		uint32_t count = 0;
		while( a_query.next() ) count++;
		if( !count ){
			a_query.prepare("INSERT INTO blackUrls (url) VALUES (:url);");
			a_query.bindValue(":url", str);
			if (!a_query.exec()) app::setLog(1,QString("app::addBlackUrl cannot add url"));
		}
	}

	QByteArray parsRequest(const QString &data)
	{
		QByteArray ba;
		QByteArray method;
		QByteArray tempBuff;
		QByteArray param;
		std::map< QByteArray, std::vector<QByteArray> > args;


		for( uint16_t i = 0; i < data.length(); i++ ){
			if( i == 0 && data[i] == '/' ) continue;
			if( i > 0 ){
				if( data[i] == '?' || data[i] == '=' || data[i] == '&' ){
					if( tempBuff.size() == 0 ) break;
				}
			}
			if( i > 0 && data[i] == '?' ){
				method.append( tempBuff );
				tempBuff.clear();
				continue;
			}

			if( i > 0 && data[i] == '=' ){
				param.append( tempBuff );
				tempBuff.clear();
				continue;
			}

			if( i > 0 && data[i] == '&' ){
				if( param.size() == 0 ) break;
				args[param].push_back( tempBuff );
				tempBuff.clear();
				continue;
			}

			tempBuff.append( data[i] );
		}

		if( param.size() > 0 && tempBuff.size() > 0 ) args[param].push_back( tempBuff );

		if( method == "get" ){
			for( auto type:args ){
				for( auto value:type.second ){
					//qDebug()<<type.first<<value<<method;
					//"con" "globalBlockedUrls" "get"
					if( type.first == "con" && value == "globalBlockedUrls" ){

					}
				}
			}
		}

		return ba;
	}

}
