
#include "Help.hpp"
#include "buildmanager.h"
#include "Include/UtilsUI.hpp"

#include "Dialogs/TexDoc.hpp"

#include <algorithm>



TexHelp::TexHelp(QObject * parent)
    : QObject(parent)
    , texDocSystem(0) {}


/*!
 *	\brief Execute a dialog to let the user choose a package to show its documentation
 *	\param packages
 *	\param defaultPackage
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
 *	\brief Run texdoc --view package
 *	\param package
 */

void TexHelp::viewTexdoc(QString package){

	if(package.isEmpty()){

		auto action = qobject_cast<QAction *>(sender());

		if(!action)
			return;

		package = action -> data().toString();
	}

    if(package.isEmpty())
		return;
	
	QString answer = runTexdoc("--view " + package);
}


/*!
 *	\brief check if system runs miktex
 *	Tries to run texdoc --veriosn and analyzes result.
 *	Miktex starts with MikTeX ...
 *	\return
 */

bool TexHelp::isMiktexTexdoc(){

	if(texDocSystem)
		return (texDocSystem == 1);

	const auto version = runTexdoc("--version");
	texDocSystem = version.startsWith("MiKTeX") + 1;

    return (texDocSystem == 1);
}


// miktex texdoc will run as long as the viewer is opened when the MIKTEX_VIEW_* variables are set
// https://docs.miktex.org/manual/mthelp.html

bool TexHelp::isTexdocExpectedToFinish(){

	if(!isMiktexTexdoc())
		return true;

	const auto keys = QProcessEnvironment::systemEnvironment().keys();

	return std::any_of(keys.begin(),keys.end(),[](auto & key){
		return key.startsWith("MIKTEX_VIEW_");
	});
}


/*!
 *	\brief search for documentation files for a given package
 *	It uses texdoc to access that information.
 *	\param package
 *	\param silent
 *	\return
 */

QString TexHelp::packageDocFile(const QString & package,bool silent){

	auto command = BuildManager::CMD_TEXDOC;

	if(command.isEmpty()){
		if(!silent)
			UtilsUi::txsWarning(tr("texdoc not found."));

		return "";
	}

	QStringList args;

	if(isMiktexTexdoc())
		args << "--list-only";
	else
		args << "--list" << "--machine";


	args << package;


	QString output = runTexdoc(args.join(" "));

	QStringList allFiles;

	if(isMiktexTexdoc()){
		allFiles = output.split("\r\n");
	} else {
		for(const auto & line : output.split("\n")){

			auto columns = line.simplified().split(" ");

			if(columns.count() > 2)
				allFiles << columns.at(2);
		}
	}

	for(const auto & file : allFiles)
		if(file.endsWith(".pdf") || file.endsWith(".dvi"))
			return file;

	return "";
}


/*!
 *	\brief Search for documentation files for a given package asynchrnously
 *	It uses texdoc to access that information.
 *	The results are processed in texdocAvailableRequestFinished
 *	\param package
 *	\param silent
 *	\return
 */

void TexHelp::texdocAvailableRequest(const QString & package){

	if(package.isEmpty())
		return;

	if(BuildManager::CMD_TEXDOC.isEmpty()){
		emit texdocAvailableReply(package,false,tr("texdoc not found."));
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

	// texdoc --list failed

    if(status != QProcess::NormalExit)
        return;

    auto process = qobject_cast<ProcessX *>(sender());
    auto buffer = process -> getStdoutBuffer();
    auto cmdLine = process -> getCommandLine();
    int i = cmdLine.lastIndexOf(" ");
    QString package;

    if(i > -1)
        package = cmdLine.mid(i + 1);

	// sanity check

    if(buffer == nullptr)
        return; 

    if(!isMiktexTexdoc() && !buffer->isEmpty()){

        // analyze texdoc --list result in more detail,
		// as it gives results even for partially matched names
        
		auto lines = buffer -> split("\n");
        auto line = lines.first();
        auto columns = line.split("\t");
        
		// only partial, no real match

		if(columns.count() > 4 && columns.value(1).startsWith("-"))
            buffer -> clear(); 
    }

    emit texdocAvailableReply(package,!buffer -> isEmpty(),"");

    delete buffer;
}


/*!
 *	\brief Run texdoc command
 *	\param args
 *	\return
 */

QString TexHelp::runTexdoc(QString args){

    QString output;

    emit statusMessage(QString(" texdoc "));
    emit runCommand(BuildManager::CMD_TEXDOC + " " + args,& output);

    return output;
}


/*!
 *	\brief Run texdoc command asynchronously
 *	\param args
 *	\param finishedCMD SLOT for return path
 *	\return
 */

bool TexHelp::runTexdocAsync(QString args,const char * finishedCMD){

    emit statusMessage(QString(" texdoc (async)"));
    emit runCommandAsync(BuildManager::CMD_TEXDOC + " " + args,finishedCMD);

    return true;
}
