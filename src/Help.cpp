

#include "Help.hpp"
#include "buildmanager.h"
#include "utilsUI.h"

#include "Dialogs/TexDoc.hpp"



TexHelp::TexHelp(QObject * parent)
    : QObject(parent)
    , texDocSystem(0) {}


/*!
 * \brief execute a dialog to let the user choose a package to show its documentation
 * \param packages
 * \param defaultPackage
 */

void TexHelp::execTexdocDialog(const QStringList & packages,const QString & defaultPackage){

	TexdocDialog dialog(nullptr,this);

	dialog.setPackageNames(packages);

	if(!defaultPackage.isEmpty())
		dialog.setPreferredPackage(defaultPackage);

	if(dialog.exec())
		viewTexdoc(dialog.selectedPackage());
}


/*!
 * \brief run texdoc --view package
 * \param package
 */

void TexHelp::viewTexdoc(QString package){

	if(package.isEmpty()){

		QAction  *act = qobject_cast<QAction *>(sender());

		if(!act)
			return;

		package = act -> data().toString();
	}

    if(!package.isEmpty())
        QString answer = runTexdoc("--view " + package);
}


/*!
 * \brief check if system runs miktex
 * Tries to run texdoc --veriosn and analyzes result.
 * Miktex starts with MikTeX ...
 * \return
 */

bool TexHelp::isMiktexTexdoc(){

    if(!texDocSystem) {
        QString answer = runTexdoc("--version");
        texDocSystem = answer.startsWith("MiKTeX") ? 1 : 2;
    }

    return (texDocSystem == 1);
}


bool TexHelp::isTexdocExpectedToFinish(){

	if(!isMiktexTexdoc())
		return true;

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	foreach(const QString &var,env.keys()){

		if(var.startsWith("MIKTEX_VIEW_")){
			// miktex texdoc will run as long as the viewer is opened when the MIKTEX_VIEW_* variables are set
			// http://docs.miktex.org/manual/mthelp.html
			return false;
		}
	}

	return true;
}


/*!
 * \brief search for documentation files for a given package
 * It uses texdoc to access that information.
 * \param package
 * \param silent
 * \return
 */

QString TexHelp::packageDocFile(const QString & package,bool silent){

	QString cmd = BuildManager::CMD_TEXDOC;

	if(cmd.isEmpty()){
		if(!silent)
			UtilsUi::txsWarning(tr("texdoc not found."));

		return QString();
	}

	QStringList args;

	if(isMiktexTexdoc()){
		args << "--list-only";
	} else {
		args << "--list" << "--machine";
	}

	args << package;


	QString output = runTexdoc(args.join(" "));

	QStringList allFiles;

	if(isMiktexTexdoc()){
		allFiles = output.split("\r\n");
	} else {
		foreach(const QString &line, output.split("\n")){

			QStringList cols = line.simplified().split(" ");

			if (cols.count() > 2)
				allFiles << cols.at(2);
		}
	}

	foreach(const QString & file,allFiles){
		if(file.endsWith(".pdf") || file.endsWith(".dvi"))
			return file;
	}

	return QString();
}


/*!
 * \brief search for documentation files for a given package asynchrnously
 * It uses texdoc to access that information.
 * The results are processed in texdocAvailableRequestFinished
 * \param package
 * \param silent
 * \return
 */

void TexHelp::texdocAvailableRequest(const QString & package){

	if(package.isEmpty())
		return;

	if(BuildManager::CMD_TEXDOC.isEmpty()){
		emit texdocAvailableReply(package, false, tr("texdoc not found."));
		return;
	}

	QStringList args;

	if(isMiktexTexdoc()){
		args << "--print-only" << package;
	} else {
		args << "--list" << "--machine" << package; // --print-only does not exist in texlive 2012, actual is response is not used either ...
		// TODO: not the right option: don't open the viewer here
		// There seems to be no option yielding only the would be called command
		// Alternative: texdoc --list -M and parse the first line for the package name
	}

	runTexdocAsync(args.join(" "),SLOT(texdocAvailableRequestFinished(int,QProcess::ExitStatus)));
}


void TexHelp::texdocAvailableRequestFinished(int,QProcess::ExitStatus status){

    if(status != QProcess::NormalExit)
        return; // texdoc --list failed

    ProcessX * proc = qobject_cast<ProcessX *>(sender());
    QString * buffer = proc -> getStdoutBuffer();
    QString cmdLine = proc -> getCommandLine();
    int i = cmdLine.lastIndexOf(" ");
    QString package;

    if(i>-1)
        package=cmdLine.mid(i+1);

    if(buffer == nullptr)
        return; // sanity check

    if(!isMiktexTexdoc() && !buffer->isEmpty()){
        // analyze texdoc --list result in more detail, as it gives results even for partially matched names
        QStringList lines=buffer->split("\n");
        QString line=lines.first();
        QStringList cols=line.split("\t");
        if(cols.count()>4){
            if(cols.value(1).startsWith("-")){
                buffer->clear(); // only partial, no real match
            }
        }
    }

    emit texdocAvailableReply(package,!buffer -> isEmpty(),QString());

    delete buffer;
}


/*!
 * \brief run texdoc command
 * \param args
 * \return
 */

QString TexHelp::runTexdoc(QString args){

    QString output;

    emit statusMessage(QString(" texdoc "));
    emit runCommand(BuildManager::CMD_TEXDOC + " " + args,&output);

    return output;
}


/*!
 * \brief run texdoc command asynchronously
 * \param args
 * \param finishedCMD SLOT for return path
 * \return
 */

bool TexHelp::runTexdocAsync(QString args,const char * finishedCMD){

    emit statusMessage(QString(" texdoc (async)"));
    emit runCommandAsync(BuildManager::CMD_TEXDOC + " " + args,finishedCMD);

    return true;
}
