#include "global.h"

#include <QDateTime>
#include <QSettings>
#include <QUrl>
//TODO: remove qdebug
#include <QDebug>



namespace app {
	Config conf;
	Status state;
	BlackList blackList;

	void loadSettings()
	{
		QSettings settings("MySoft","WebProxy");

		app::conf.maxThreads = settings.value("SERVER/maxThreads",app::conf.maxThreads).toUInt();
		app::conf.port = settings.value("SERVER/port",app::conf.port).toUInt();
		app::conf.blackUrlsFile = settings.value("SERVER/blackUrlsFile",app::conf.blackUrlsFile).toString();
		app::conf.blackAddrsFile = settings.value("SERVER/blackAddrsFile",app::conf.blackAddrsFile).toString();


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

		app::loadBlackList( app::conf.blackUrlsFile, app::blackList.urls );
		app::loadBlackList( app::conf.blackAddrsFile, app::blackList.addrs );

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

	void saveSettings()
	{
		if( app::conf.saveSettings ){
			QSettings settings("MySoft","WebProxy");
			settings.clear();
			settings.setValue("SERVER/maxThreads",app::conf.maxThreads);
			settings.setValue("SERVER/port",app::conf.port);
			settings.setValue("SERVER/blackUrlsFile",app::conf.blackUrlsFile);
			settings.setValue("SERVER/blackAddrsFile",app::conf.blackAddrsFile);
			app::conf.saveSettings = false;
		}
		if( app::blackList.urlsFileSave ){
			app::saveBlackList( app::conf.blackUrlsFile, app::blackList.urls );
			app::blackList.urlsFileSave = false;
		}
		if( app::blackList.addrsFileSave ){
			app::saveBlackList( app::conf.blackAddrsFile, app::blackList.addrs );
			app::blackList.addrsFileSave = false;
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

	void loadBlackList(const QString &fileName, std::vector<QString> &data)
	{
		QFile file;
		QByteArray buff;
		file.setFileName( fileName );
		if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
			QByteArray str;
			char sym;
			while(!file.atEnd()){
				file.read( &sym, 1 );
				if( sym == '\n' ){
					data.push_back( str );
					str.clear();
				}
				str.append( sym );
			}
			file.close();
		}
	}

	void saveBlackList(const QString &fileName, const std::vector<QString> &data)
	{
		QFile file;
		file.setFileName( fileName );
		if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
			for( auto elem:data ) file.write( elem.toLatin1() );
			file.close();
		}
	}

	void addBlackUrl(const QString &str)
	{
		bool find = false;

		for( auto elem:app::blackList.urls ){
			if( elem == str ){
				find = true;
				break;
			}
		}

		if( !find ){
			app::blackList.urls.push_back( str );
			app::blackList.urlsFileSave = true;
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
			ba.append("content:>:");
			for( auto type:args ){
				for( auto value:type.second ){
					//qDebug()<<type.first<<value<<method;
					//"con" "globalBlockedUrls" "get"
					if( type.first == "con" && value == "globalBlockedUrls" ){
						ba.append("globalBlockedUrls:>:");
						ba.append("<table>");
						for( auto elem:app::blackList.urls ){
							ba.append("<tr>");
							ba.append( QString("<td>" + elem + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "threads" ){
						ba.append("threads:>:");
						ba.append("<table>");
						for( uint8_t i = 0; i < app::state.threads.size(); i++ ){
							ba.append("<tr>");
							ba.append( QString("<td>Thread #" + QString::number( i ) + ":</td><td>" + QString::number( app::state.threads.at( i ) ) + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "openUrls" ){
						ba.append("openUrls:>:");
						ba.append("<table>");
						for( auto url:app::state.urls ){
							ba.append("<tr>");
							ba.append( QString("<td>" + url + "</td><td></td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "openAddrs" ){
						ba.append("openAddrs:>:");
						ba.append("<table>");
						for( auto addr:app::state.addrs ){
							ba.append("<tr>");
							ba.append( QString("<td>" + addr + "</td><td></td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "connections" ){
						ba.append("connections:>:");
						ba.append("<table>");
						for( auto user:app::conf.users ){
							ba.append("<tr>");
							ba.append( QString("<td>" + user.login + "</td><td> " + QString::number( user.connections ) + "</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
					if( type.first == "con" && value == "activeUsers" ){
						ba.append("activeUsers:>:");
						QDateTime dt = QDateTime::currentDateTime();
						ba.append("<table>");
						for( auto user:app::conf.users ){
							uint32_t lastLoginSec = dt.toTime_t() - user.lastLoginTimestamp;
							ba.append("<tr>");
							ba.append( QString("<td>" + user.login + "</td><td> " + QString::number( lastLoginSec ) + " sec. ago</td>") );
							ba.append("</tr>");
						}
						ba.append("</table>");
					}
				}
			}
		}

		return ba;
	}

	void getUserData(User &userData, const QString &login)
	{
		for( auto user:app::conf.users ){
			if( login == user.login ){
				userData = user;
				break;
			}
		}
	}

}
