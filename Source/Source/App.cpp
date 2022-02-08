

#include "App.hpp"
#include "TexStudio.hpp"
#include "debughelper.h"

#include <QSplashScreen>


const auto environment = QProcessEnvironment::systemEnvironment();


inline QString userId(){

    if(environment.contains("USER"))
        return environment.value("USER");

    if(environment.contains("USERNAME"))
        return environment.value("USERNAME");

    return "UnknownUser";
}

inline QString generateId(){
	return QString("%1_%2")
		.arg(TEXSTUDIO)
		.arg(userId());
}


App::App(int & count,char ** arguments)
	: QtSingleApplication(generateId(),count,arguments) {}

App::~App(){
	delete studio;
}


/*!
 *	@brief Inizialize MexStudio & it's events.
 */

void App::startup(QStringList & commandLine){
	
	showSplashScreen([ & ](QSplashScreen * screen){

		processEvents();

		studio = new Texstudio(nullptr,Qt::WindowFlags(),screen);

		registerShutdown();
	});

	running = true;

	if(!fileToLoadOnceRunning.isEmpty())
		commandLine << fileToLoadOnceRunning;

	studio -> executeCommandLine(commandLine,true);

	if(!commandLine.contains("--auto-tests"))
		studio -> startupCompleted();

	registerIntercom();
}


/*!
 *	@brief Show slash screen until setup is finished.
 */

void App::showSplashScreen(std::function<void(QSplashScreen *)> setup){
	
    const QPixmap image { ":/images/splash.png" };
	const auto screen = new QSplashScreen(image);

	screen -> show();

	setup(screen);

	screen -> finish(studio);

	delete screen;
}


/*!
 *	@brief Quit MexStudio on closing of last window.
 */

void App::registerShutdown(){
	connect(this,SIGNAL(lastWindowClosed()),this,SLOT(quit()));
}

/*!
 *  @brief Establish communication between instances.
 */

void App::registerIntercom(){
    connect(this,SIGNAL(messageReceived(QString)),this -> studio,SLOT(onOtherInstanceMessage(QString)));
}


/*!
 *  @brief Handle Events
 */

bool App::event(QEvent * event){

    switch(event -> type()){
    case QEvent::FileOpen:
        handleFileOpen(static_cast<QFileOpenEvent *>(event));
        return true;
    default:
        return QApplication::event(event);
    }

	return QApplication::event(event);
}


/*!
 *  @brief Handle opening of files.
 */

void App::handleFileOpen(QFileOpenEvent * event){
    
    const auto file = event -> file();

    if(running)
        studio -> load(file);
    else
        fileToLoadOnceRunning = file;

    event -> accept();
}


/*!
 *  @brief Enter execution & handle exceptions
 */

int App::run(){
	try {

		const int status = exec();

		#ifdef DEBUG_LOGGER

			if(DebugLogger::isLogging())
				DebugLogger::Stop();

		#endif

		return status;
	} catch (...) {

		#ifndef NO_CRASH_HANDLER
			catchUnhandledException();
		#endif

		throw;
	}
}


/*!
 *  @brief Have an existing instance handle the request.
 */

void App::instructOthers(const QStringList & commandLine){
    sendMessage(commandLine.join("#!#"));
}
