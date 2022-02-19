
#include "cleandialog.h"
#include "ui_cleandialog.h"

#include "configmanager.h"
#include "Include/UtilsUI.hpp"


QString CleanDialog::defaultExtensions = "log,aux,dvi,lof,lot,bit,idx,glo,bbl,bcf,ilg,toc,ind,out,blg,fdb_latexmk,fls,run.xml";
QString CleanDialog::currentExtensions = CleanDialog::defaultExtensions;
int CleanDialog::scopeID = 0;


static const int AbsFilePathRole = Qt::UserRole;


CleanDialog::CleanDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::CleanDialog){

	ui -> setupUi(this);
	UtilsUi::resizeInFontHeight(this,38,22);

	auto * config = ConfigManager::getInstance();
	config -> registerOption("CleanDialog/Extensions",& currentExtensions,defaultExtensions);
	config -> registerOption("CleanDialog/Scope",& scopeID,0);

	if(scopeID < 0 || scopeID >= MAX_SCOPE)
		scopeID = 0;

	QString allowedChars = "[^\\\\/\\?\\%\\*:|\"<>\\s,;]";
	QRegularExpressionValidator * rxValExtensionList = new QRegularExpressionValidator(
		QRegularExpression(QString("(%1+\\.)*%1+(,(%1+\\.)*%1+)*").arg(allowedChars)),this);

	int dummyPos;

	if(rxValExtensionList -> validate(currentExtensions,dummyPos) != QValidator::Acceptable){
		UtilsUi::txsWarning("Invalid extension list found. Resetting to default.");
		currentExtensions = defaultExtensions;
	}

	auto extensions = ui -> leExtensions;
	extensions -> setText(currentExtensions);
	extensions -> setValidator(rxValExtensionList);

	ui -> pbResetExtensions->setIcon(getRealIcon("edit-undo"));

	connect(ui -> pbResetExtensions,SIGNAL(clicked()),SLOT(resetExtensions()));
	connect(ui -> cbScope,SIGNAL(currentIndexChanged(int)),SLOT(updateFilesToRemove()));
	connect(ui -> leExtensions,SIGNAL(editingFinished()),SLOT(updateFilesToRemove()));
	connect(ui -> buttonBox,SIGNAL(accepted()),SLOT(onAccept()));
	connect(ui -> buttonBox,SIGNAL(rejected()),SLOT(onReject()));
}


CleanDialog::~CleanDialog(){
	delete ui;
}


/* internally sets up possible clean variants. Call before exec()
   Returns true, if at least one variant can be applied. */

bool CleanDialog::checkClean(const LatexDocuments & documents){

	auto scope = ui -> cbScope;
	const auto last = scope -> count() - 1;


	bool somethingToClean = false;

	// don't emit currentIndexChanged while populating the combo box

	scope -> blockSignals(true);


	const auto master = documents.currentDocument;

	if(master){

		masterFile = master -> getFileName();

		scope -> addItem(tr("Project (Master file folder and all subfolders)"),CleanDialog::Project);

		if(scopeID == CleanDialog::Project)
			scope -> setCurrentIndex(scope -> count() - 1);

		somethingToClean = true;
	}

	const auto current = documents.currentDocument;

	if(current){

		const auto extension = current 
			-> getFileInfo()
			.  suffix()
			.  toLower();
			
		if(extension == "tex"){

			currentTexFile = current -> getFileName();

			scope -> addItem(tr("Current File"),CleanDialog::CurrentTexFile);

			if(scopeID == CleanDialog::CurrentTexFile)
				scope-> setCurrentIndex(last);

			ui -> cbScope -> addItem(tr("Current File Folder"),CleanDialog::CurrentFileFolder);

			if(scopeID == CleanDialog::CurrentFileFolder)
				scope -> setCurrentIndex(last);

			somethingToClean = true;
		}
	}


	for(const auto & document : documents.documents){

		const auto extension = document 
			-> getFileInfo()
			.  suffix()
			.  toLower();

		if(extension == "tex")
			openTexFiles.append(document -> getFileName());
	}


	if(!openTexFiles.isEmpty()){

		scope -> addItem(tr("Open Files"),CleanDialog::OpenTexFiles);

		if(scopeID == CleanDialog::OpenTexFiles)
			scope -> setCurrentIndex(last);

		somethingToClean = true;
	}


	scope -> blockSignals(false);

	if(somethingToClean)
		updateFilesToRemove();

	return somethingToClean;
}


void CleanDialog::resetExtensions(){
	
	ui 
		-> leExtensions 
		-> setText(defaultExtensions);
}


void CleanDialog::onAccept(){

	const auto & files = ui -> lwFiles;

	for(int i = 0;i < files -> count();i++){

		const auto path = files 
			-> item(i) 
			-> data(AbsFilePathRole)
			.  toString();

		QFile::remove(path);
	}

	accept();
}


void CleanDialog::onReject(){
	reject();
}


void CleanDialog::updateFilesToRemove(){

	const auto s = ui -> cbScope;

	scopeID = s 
		-> itemData(s -> currentIndex())
		.  toInt();

	Scope scope = (Scope) scopeID;

	const auto extensions = ui -> leExtensions;

	QStringList extList(extensions -> text().split(',',Qt::SkipEmptyParts));

	QStringList forbiddenExtensions = QStringList() 
		<< "tex" 
		<< "lytex";
	
	QStringList found;

	for(const auto & extension : forbiddenExtensions)
		if(extList.contains(extension))
			found << extension;

	if(!found.isEmpty()){

		UtilsUi::txsWarning(tr("For your own safety clean will not delete the files with the following extensions:") + QString("\n") + extList.join(", "));

		for(const auto & extension : found)
			extList.removeAll(extension);
	}

	currentExtensions = extList.join(',');
	extensions -> setText(currentExtensions);

	QStringList removeable;
	removeable << filesToRemove(scope,extList);


	auto files = ui -> lwFiles;

	files -> clear();

	for(const auto & path : removeable){

		QFileInfo info(path);

		auto item = new QListWidgetItem(info.fileName());
		item -> setData(AbsFilePathRole,path);
		item -> setToolTip(path);

		files -> addItem(item);
	}
}


QStringList CleanDialog::filesToRemove(CleanDialog::Scope scope,const QStringList & extensionFilter){

	QStringList files;

	switch(scope){
	case Project :{

		QStringList filterList;

		for(const auto & extension : extensionFilter)
			filterList << "*." + extension;

		const auto path = QFileInfo(masterFile)
			.absoluteDir();

		files << filesToRemoveFromDir(path,filterList);

		break;
	}
	case CurrentTexFile :{

		QFileInfo info(currentTexFile);
		const auto basename = info.absolutePath() + "/" + info.completeBaseName();

		for(const auto & extension : extensionFilter){

			QFileInfo info(basename + "." + extension);

			if(info.exists())
				files << info.absoluteFilePath();
		}

		break;
	}
	case CurrentFileFolder :{

		QStringList filterList;

		for(const auto & extension : extensionFilter)
			filterList << "*." + extension;

		const auto path = QFileInfo(currentTexFile)
			.absoluteDir();

		files << filesToRemoveFromDir(path,filterList);
	
		break;
	}
	case OpenTexFiles :{

		for(const auto & path : openTexFiles){

			QFileInfo info(path);
			const auto basename = info.absolutePath() + "/" + info.completeBaseName();

			for(const auto & extension : extensionFilter){

				QFileInfo info(basename + "." + extension);

				if(info.exists())
					files << info.absoluteFilePath();
			}
		}

		break;
	}
	case None:
		break;
	default:
		break;
	}

	return files;
}


QStringList CleanDialog::filesToRemoveFromDir(
	const QDir & dir,
	const QStringList & extensionFilter,
	bool recursive
){

	QStringList files;

	const auto infos = dir.entryInfoList(extensionFilter,QDir::Files);

	for(const auto & info : infos)
		files << info.absoluteFilePath();

	if(recursive){

		const auto infos = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);

		for(const auto & info : infos){

			// filter directories starting with . (e.g. .git) 
			// since they are not automatiocally hidden on windows

			const bool isHidden = info
				.fileName()
				.startsWith('.');

            if(isHidden)
                continue;

			files << filesToRemoveFromDir(info.absoluteFilePath(),extensionFilter,recursive);
		}
	}

	return files;
}
