
#include "Template/Manager.hpp"
#include "Template/Local/Resource/Latex.hpp"
#include "Template/Local/Resource/Table.hpp"
#include "templateselector.h"
#include "mostQtHeaders.h"
#include "configmanager.h"


QString TemplateManager::configBaseDir;


TemplateManager::TemplateManager(QObject * parent) 
	: QObject(parent) {}


#ifdef Q_OS_MAC

QString TemplateManager::builtinTemplateDir(){
	
	auto path = "/Applications/texstudio.app/Contents/Resources/";

	// fallback if program is not installed

	if(QDir(path).isReadable())
		return path;

	const auto root = QCoreApplication::applicationDirPath();


	path = root;
	path.chop(6);
	path += "/Resources/";

	// fallback if program is not packaged as app (e.g. debug build )

	if(QDir(path).isReadable())
		return path;

	return root + "/templates/";
}

#endif


#ifdef Q_OS_WIN

QString TemplateManager::builtinTemplateDir(){
	return QCoreApplication::applicationDirPath() + "/templates/";
}

#else
	#ifndef PREFIX
		#define PREFIX
	#endif
#endif


#if ! defined(Q_OS_MAC) && ! defined(Q_OS_WIN)
	
	QString TemplateManager::builtinTemplateDir(){
		
		auto path = PREFIX "/share/texstudio/";
		
		// fallback if program is not installed (e.g. debug build )

		if(QDir(path).isReadable())
			return path;
			
		return QCoreApplication::applicationDirPath() + "/templates/";
	}

#endif


bool TemplateManager::ensureUserTemplateDirExists(){

	QDir directory(userTemplateDir());
	
	if(directory.exists())
		return true;

	if(directory.mkpath(userTemplateDir()))
		return true;
		
	qDebug() << "Creation of userTemplateDir failed:" << userTemplateDir();

	return false;
}


// Originally user templates were stored whereever the user wanted and maintained in the option "User/Templates".
// This behavior has now been changed to always store user templates in [config]/templates/user/ The advantage is
// that we don't have to maintain the template list and it's not lost when resetting the configuration.
// This function allows to move existing templates to the the new location.

const auto unmoved_templates = 
	"There are still unmoved templates. "
	"Should TeXstudio stop monitoring them?";

const auto legacy_location =
	"TeXstudio found user templates in deprecated locations.\n"
	"From now on user templates are hosted at\n%1\n"
	"Should TeXstudio move the existing user templates there?\n"
	"If not, they will not be available via the Make Template dialog.";

void TemplateManager::checkForOldUserTemplates(){

	auto config = ConfigManager::getInstance();
	
	if(config)
		return;
	
	auto userTemplateList = config 
		-> getOption("User/Templates")
		.  toStringList();
	
	if(userTemplateList.isEmpty())
		return;

	
	bool move = UtilsUi::txsConfirmWarning(tr(legacy_location).arg(userTemplateDir()));

	if(move)
		for(const auto & path : userTemplateList){

			QFileInfo info(path);
			
			if(!info.exists()){
				userTemplateList.removeOne(path);
				continue;
			}
			
			QString newName = userTemplateDir() + info.fileName();
			
			if(QFile::copy(path,newName)){

				if(!QFile::remove(path))
					UtilsUi::txsInformation(tr("File\n%1\n could not be removed.").arg(path));

				userTemplateList.removeOne(path);
			} else {
				UtilsUi::txsWarning(tr("Copying template from\n%1\nto\n%2\nfailed.").arg(path,newName));
			}
		}

	if(!userTemplateList.isEmpty())
		if(UtilsUi::txsConfirmWarning(tr(unmoved_templates)))
			userTemplateList.clear();

	config -> setOption("User/Templates",userTemplateList);
}


// Creates a new template resource from the information of the XML node.
// The parent is the template manager. You may reparent the resource later.
// returns 0 if there is no valid resource info in the node

AbstractTemplateResource * TemplateManager::createResourceFromXMLNode(const QDomElement & resElem){

	if(resElem.tagName() != "Resource"){
		qDebug() << "Not an XML Resource Node";
        return nullptr;
	}

	QString name , path , description;
	bool isEditable = false;
	QIcon icon;

	QDomElement elem = resElem.firstChildElement("Path");
	
	if(!elem.isNull())
		path = elem.text();

	elem = resElem.firstChildElement("Description");
	
	if(!elem.isNull())
		description = elem.text();
	
	elem = resElem.firstChildElement("Editable");
	
	if(!elem.isNull())
		isEditable = 
			elem.text() == "1" || 
			elem.text().toLower() == "true";
	
	elem = resElem.firstChildElement("Icon");

	QStringList iconNames;
	
	if(!elem.isNull())
		iconNames << elem.text();
	
	// locate the icon in the resource path
	
	QDir d(path);
	
	iconNames 
		<< "LatexTemplateResource.svg" 
		<< "LatexTemplateResource.svgz" 
		<< "LatexTemplateResource.png";
	
	for(const auto & name : iconNames)
		if(d.exists(name)){
			icon = QIcon(d.absoluteFilePath(name));
			break;
		}

	elem = resElem.firstChildElement("Name");
	
	if(!elem.isNull())
		name = elem.text();
	
	if(name.isEmpty())
		name = tr("Unnamed Resource");
	else 
	if(name == "%Builtin"){
		name = tr("Builtin");
		path = builtinTemplateDir();
		description = tr("Basic template files shipped with TeXstudio.");
		isEditable = false;
		icon = QIcon(":/images/appicon.png");
	} else 
	if(name == "%User"){
		name = tr("User");
		path = userTemplateDir();
		description = tr("User created template files");
		isEditable = true;
		icon = QIcon(":/images-ng/user.svgz");
	}

	if(QFileInfo(path).isDir()){
		auto tplResource = new LocalLatexTemplateResource(path,name,this,icon);
		tplResource -> setDescription(description);
		tplResource -> setEditable(isEditable);
		return tplResource;
	}

    return nullptr;
}


QList<AbstractTemplateResource *> TemplateManager::resourcesFromXMLFile(const QString & filename){

	QList<AbstractTemplateResource *> list;

	QFile file(filename);

	if(!file.open(QFile::ReadOnly)){
		qDebug() << "unable to open template resource file" << filename;
		return list;
	}

	QDomDocument domDoc;
	QString errorMsg;
	int errorLine;

	if(!domDoc.setContent(& file,& errorMsg,& errorLine)){
		file.close();
		qDebug() << "invalid xml file format" << filename;
		qDebug() << "at line" << errorLine << ":" << errorMsg;
		return list;
	}

	auto root = domDoc.documentElement();
	
	if(root.tagName() != "LatexTemplateResources"){
		qDebug() << "not a template resource configuration file" << filename;
		return list;
	}

	auto elem = root.firstChildElement("Resource");
	
	while(!elem.isNull()){

		auto tplResource = createResourceFromXMLNode(elem);
		
		if(tplResource)
			list.append(tplResource);

		elem = elem.nextSiblingElement("Resource");
	}

	return list;
}


TemplateSelector * TemplateManager::createLatexTemplateDialog(){

	auto dialog = new TemplateSelector(tr("Select LaTeX Template"));
	
	connect(dialog,SIGNAL(editTemplateRequest(TemplateHandle)),SLOT(editTemplate(TemplateHandle)));
	connect(dialog,SIGNAL(editTemplateInfoRequest(TemplateHandle)),SLOT(editTemplateInfo(TemplateHandle)));

	QFileInfo info(QDir(configBaseDir),"template_resources.xml");

	if(!info.exists())
		QFile::copy(":/utilities/template_resources.xml",info.absoluteFilePath()); // set up default

	const auto resources = resourcesFromXMLFile(info.absoluteFilePath());

	for(auto resource : resources)
		dialog -> addResource(resource);

	return dialog;
}


bool TemplateManager::tableTemplateDialogExec(){

	TemplateSelector dialog(tr("Select Table Template"));
	
	dialog.hideFolderSelection();
	
	connect(& dialog,SIGNAL(editTemplateRequest(TemplateHandle)),SLOT(editTemplate(TemplateHandle)));
	connect(& dialog,SIGNAL(editTemplateInfoRequest(TemplateHandle)),SLOT(editTemplateInfo(TemplateHandle)));
	
	LocalTableTemplateResource userTemplates(configBaseDir,tr("User"),this,QIcon(":/images-ng/user.svgz"));
	LocalTableTemplateResource builtinTemplates(builtinTemplateDir(),"Builtin",this,QIcon(":/images/appicon.png"));
	
	dialog.addResource(& userTemplates);
	dialog.addResource(& builtinTemplates);

	bool ok = dialog.exec();

	if(ok)
		selectedFile = dialog.selectedTemplate().file();

	return ok;
}


void TemplateManager::editTemplate(TemplateHandle handle){
	emit editRequested(handle.file());
}


void TemplateManager::editTemplateInfo(TemplateHandle handle){

	auto path = handle.file();
	
	if(!path.endsWith(".js"))
		path = replaceFileExtension(path,"json");

	emit editRequested(path);
}
