/***************************************************************************
 *   copyright       : (C) 2003-2007 by Pascal Brachet                     *
 *   addons by Frederic Devernay <frederic.devernay@m4x.org>               *
 *   addons by Luis Silvestre                                              *
 *   http://www.xm1math.net/texmaker/                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mostQtHeaders.h"

/*! \mainpage TexStudio
 *
 * \see Texstudio
 * \see PDFDocument
 */

#include "texstudio.h"
#include "smallUsefulFunctions.h"
#include "debughelper.h"
#include "debuglogger.h"
#include "utilsVersion.h"
#include <qtsingleapplication.h>
#include <QSplashScreen>
#include <iostream>

#ifdef Q_OS_WIN32
	#include "windows.h"
	typedef BOOL (WINAPI *AllowSetForegroundWindowFunc)(DWORD);
#endif


class TexstudioApp : public QtSingleApplication {

	public:

		QString delayedFileLoad;
		Texstudio * mw;
		bool initialized;

		TexstudioApp(int & args,char ** arguments);
		TexstudioApp(QString & id,int & args,char ** arguments);
		~TexstudioApp();

		void init(QStringList & cmdLine);

	protected:

		bool event(QEvent * event);
};


TexstudioApp::TexstudioApp(int & args,char ** arguments)
	: QtSingleApplication(args,arguments){

	mw = nullptr;
	initialized = false;
}


TexstudioApp::TexstudioApp(QString & id,int & args,char ** arguments)
	: QtSingleApplication(id,args,arguments){

	mw = nullptr;
	initialized = false;
}


void TexstudioApp::init(QStringList & cmdLine){

	QPixmap pixmap(":/images/splash.png");
	QSplashScreen * splash = new QSplashScreen(pixmap);

	splash -> show();
	processEvents();

	mw = new Texstudio(nullptr,Qt::WindowFlags(),splash);

	connect(this,SIGNAL(lastWindowClosed()),this,SLOT(quit()));

	splash->finish(mw);

	delete splash;

	initialized = true;

	if(!delayedFileLoad.isEmpty())
		cmdLine << delayedFileLoad;

	mw -> executeCommandLine(cmdLine,true);

	if(!cmdLine.contains("--auto-tests"))
		mw->startupCompleted();
}


TexstudioApp::~TexstudioApp(){
	delete mw;
}


bool TexstudioApp::event(QEvent * event){

	if(event -> type() == QEvent::FileOpen){

		QFileOpenEvent * openEvent = static_cast<QFileOpenEvent *>(event);

		if(initialized)
			mw->load(openEvent -> file());
		else
			delayedFileLoad = openEvent -> file();

		event->accept();

		return true;
	}

	return QApplication::event(event);
}


QString generateAppId(){

	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	QString user = environment.value("USER");

	if(user.isEmpty())
		user = environment.value("USERNAME");

    return QString("%1_%2").arg(TEXSTUDIO,user);
}


QStringList parseArguments(const QStringList & args,bool & outStartAlways){

	QStringList cmdLine;

	for(int i = 1;i < args.count();++i){

		QString cmdArgument =  args[i];

		if(cmdArgument.startsWith('-')){
			// various commands
			if (cmdArgument == "--start-always")
				outStartAlways = true;
			else if (cmdArgument == "--no-session")
				ConfigManager::dontRestoreSession = true;
			else if ((cmdArgument == "-line" || cmdArgument == "--line") && (++i < args.count()))
				cmdLine << "--line" << args[i];

			else if ((cmdArgument == "-page" || cmdArgument == "--page") && (++i < args.count()))
				cmdLine << "--page" << args[i];

			else if ((cmdArgument == "-insert-cite" || cmdArgument == "--insert-cite") && (++i < args.count()))
				cmdLine << "--insert-cite" << args[i];

			else if (cmdArgument == "--ini-file" && (++i < args.count())) {
				// deprecated: use --config instead
				ConfigManager::configDirOverride = QFileInfo(args[i]).absolutePath();
			}
			else if (cmdArgument == "--config" && (++i < args.count()))
				ConfigManager::configDirOverride = args[i];
		#ifdef DEBUG_LOGGER
			else if ((cmdArgument == "--debug-logfile") && (++i < args.count()))
				debugLoggerStart(args[i]);
		#endif
			else
				cmdLine << cmdArgument;
        } else {
            if(cmdArgument.endsWith(".txss")){
                // explicit session restor
                // disable restoring of last session
                ConfigManager::dontRestoreSession = true;
            }
			cmdLine << QFileInfo(cmdArgument).absoluteFilePath();
        }
	}

	return cmdLine;
}


bool handleCommandLineOnly(const QStringList & cmdLine){

	// note: stdout is not supported for Win GUI applications. Will simply not output anything there.
	if (cmdLine.contains("--help")) {
		QTextStream(stdout) << "Usage: texstudio [options] [file]\n"
							<< "\n"
							<< "Options:\n"
							<< "  --config DIR              use the specified settings directory\n"
							<< "  --master                  define the document as explicit root document\n"
							<< "  --line LINE[:COL]         position the cursor at line LINE and column COL\n"
							<< "  --insert-cite CITATION    inserts the given citation\n"
							<< "  --start-always            start a new instance, even if TXS is already running\n"
							<< "  --pdf-viewer-only         run as a standalone pdf viewer without an editor\n"
							<< "  --page PAGENUM            display a certain page in the pdf viewer\n"
                            << "  --no-session              do not load/save the session at startup/close\n"
                            << "  --texpath PATH            force resetting command defaults with PATH as first search path"
                            << "  --version                 show version number\n"
#ifdef DEBUG_LOGGER
							<< "  --debug-logfile pathname  write debug messages to pathname\n"
#endif
							;
		return true;
	}

    if(cmdLine.contains("--version")){
        QTextStream(stdout) << "TeXstudio " << TXSVERSION << " (" << TEXSTUDIO_GIT_REVISION << ")\n";
		return true;
	}

	return false;
}


int main(int args,char ** argument){

	#if __cplusplus >= 202002L
		std::cout << "C++ 20" << std::endl;
	#endif

	std::cout << "C++ " << __cplusplus << std::endl;

	QString appId = generateAppId();

		QApplication::setAttribute(
			(qEnvironmentVariableIntValue("TEXSTUDIO_HIDPI_SCALE") > 0)
			? Qt::AA_EnableHighDpiScaling
			: Qt::AA_DisableHighDpiScaling
		);

	// This is a dummy constructor so that the programs loads fast.
	TexstudioApp a(appId,args,argument);
	bool startAlways = false;
	QStringList cmdLine = parseArguments(QCoreApplication::arguments(), startAlways);

	if (handleCommandLineOnly(cmdLine)) {
		return 0;
	}

	if (!startAlways) {
		if (a.isRunning()) {
		#ifdef Q_OS_WIN32
			AllowSetForegroundWindowFunc asfw = (AllowSetForegroundWindowFunc) GetProcAddress(GetModuleHandleA("user32.dll"), "AllowSetForegroundWindow");
			if (asfw) asfw(/*ASFW_ANY*/(DWORD)(-1));
		#endif
			a.sendMessage(cmdLine.join("#!#"));
			return 0;
		}
	}

	a.setApplicationName( TEXSTUDIO );

		a.setDesktopFileName("texstudio");

	a.init(cmdLine); // Initialization takes place only if there is no other instance running.

    QObject::connect(
        &a,SIGNAL(messageReceived(QString)),
        a.mw,SLOT(onOtherInstanceMessage(QString)));

	try {
		int execResult = a.exec();

		#ifdef DEBUG_LOGGER
		if (debugLoggerIsLogging()) {
			debugLoggerStop();
		}
		#endif

		return execResult;
	} catch (...) {

		#ifndef NO_CRASH_HANDLER
			catchUnhandledException();
		#endif

		throw;
	}
}
