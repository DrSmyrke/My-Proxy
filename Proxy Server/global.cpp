#include "global.h"

#include <QDateTime>
#include <QSettings>
#include <QDebug>

namespace app {
	Config conf;

	void loadSettings()
	{
		QSettings settings("MySoft","WebProxy");

		app::conf.maxThreads = settings.value("SERVER/maxThreads",app::conf.maxThreads).toUInt();
		app::conf.port = settings.value("SERVER/port",app::conf.port).toUInt();

		QFile ftop(":///pages/top.html");
		if(ftop.open(QIODevice::ReadOnly | QIODevice::Text)){
			while (!ftop.atEnd()) app::conf.page.top.append( ftop.readAll() );
			ftop.close();
		}
		QFile fbottom(":///pages/bottom.html");
		if(fbottom.open(QIODevice::ReadOnly | QIODevice::Text)){
			while (!fbottom.atEnd()) app::conf.page.bottom.append( fbottom.readAll() );
			fbottom.close();
		}
		QFile fmenu(":///pages/menu.html");
		if(fmenu.open(QIODevice::ReadOnly | QIODevice::Text)){
			while (!fmenu.atEnd()) app::conf.page.menu.append( fmenu.readAll() );
			fmenu.close();
		}
	}

	void saveSettings()
	{
		QSettings settings("MySoft","WebProxy");
		settings.clear();

		settings.setValue("SERVER/maxThreads",app::conf.maxThreads);
		settings.setValue("SERVER/port",app::conf.port);
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
		//TODO: Реализовать добавление в БД
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
		for( auto user:app::conf.users){
			if( user.login == login ){
				if( app::passIsValid( pass, user.pass ) ) res = true;
				break;
			}
		}
		return res;
	}

}
