
#include "Template/Template.hpp"
#include <Include/UtilsUI.hpp>
#include <JlCompress.h>


const auto warning_nonEmpty = 
	"The target folder is not empty. It is recommended to instantiate "
	"in new folders. Otherwise existing files may be overwritten. "
	"Do you wish to use this folder anyway?";


bool Template::createInFolder(const QString & path){

	QDir dir(path);
	
	if(!dir.exists()){
		bool created = dir.mkpath(".");
		
		if(!created)
			return false;
	} else {
		
		QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
		
		if(!entries.isEmpty()){

			const auto warning = QCoreApplication::translate("TemplateManager",warning_nonEmpty);
			const bool ok = UtilsUi::txsConfirmWarning(warning);
			
			if(!ok)
				return false;
		}
	}

	if(!isMultifile()){
		QFileInfo info(file());
		QFileInfo target(dir,info.fileName());
		return QFile::copy(file(),target.absoluteFilePath());
	} else {
		bool success = ! JlCompress::extractDir(file(),dir.absolutePath()).isEmpty();
		return success;
	}
}
