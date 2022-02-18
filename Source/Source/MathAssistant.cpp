#include "MathAssistant.hpp"
#include "smallUsefulFunctions.h"
#include <QMutex>

MathAssistant * MathAssistant::m_Instance = nullptr;

MathAssistant::MathAssistant() 
	: QObject() {

	connect(& process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(processError(QProcess::ProcessError)));
	connect(& process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
}


void MathAssistant::exec(){

	if(process.state() == QProcess::Running)
		return;

	// return value of TexTablet is always zero, so we have to decide on the clipboard contents, if TexTablet was aborted
	
	auto clipboard = QApplication::clipboard();

	lastClipboardText = clipboard -> text();
	clipboard -> clear();

	//TODO This is the default installation path of TexTablet. Make it configurabe.
	
	QString texTablet = QCoreApplication::applicationDirPath() + "/TexTablet/TeXTablet.exe";

	if(!QFileInfo(texTablet).exists()){

		QString msg = QString(tr("TexTablet not found."));
		
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Math Assistant"));
		msgBox.setTextFormat(Qt::RichText);
		msgBox.setText(msg);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
		
		return;
	}
	process.setWorkingDirectory(QFileInfo(texTablet).absolutePath());
    process.start("\"" + texTablet + "\"",QStringList());
}


const std::map<QProcess::ProcessError,QString> errors {
	{ QProcess::FailedToStart , "FailedToStart" } ,
	{ QProcess::UnknownError  , "UnknownError"  } ,
	{ QProcess::WriteError    , "WriteError"    } ,
	{ QProcess::ReadError     , "ReadError"     } ,
	{ QProcess::Timedout      , "Timedout"      } ,
	{ QProcess::Crashed       , "Crashed"       }
};

void MathAssistant::processError(QProcess::ProcessError error){

	QString message = "default";

	if(errors.contains(error))
		message = errors.at(error);

	QMessageBox::information(nullptr,message,message);
}


void MathAssistant::processFinished(int code,QProcess::ExitStatus status){
	
	Q_UNUSED(code)

	if(status != QProcess::NormalExit)
		UtilsUi::txsCritical(tr("TexTablet crashed."));

	// exit code is always zero for TexTablet

	auto clipboard = QApplication::clipboard();

	QString text = clipboard -> text();
	
	if(text.startsWith("$"))
		emit formulaReceived(text);
	
	clipboard -> setText(lastClipboardText);
}


MathAssistant * MathAssistant::instance(){

	static QMutex mutex;
	mutex.lock();
	
	if(!m_Instance)
		m_Instance = new MathAssistant();
	
	mutex.unlock();
	
	return m_Instance;
}
