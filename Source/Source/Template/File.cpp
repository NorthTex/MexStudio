
#include "Template/Local/File.hpp"
#include "smallUsefulFunctions.h"


QDate LocalFileTemplate::date() const {
	return QDate::fromString(metaData["Date"],Qt::ISODate);
}


QStringList LocalFileTemplate::filesToOpen() const {

	QStringList files;
	
    for(auto & file : metaData["FilesToOpen"].split(";")){
		file = file.trimmed();
		
        if(!file.isEmpty())
			files << file;
	}
	
    return files;
}


LocalFileTemplate::LocalFileTemplate(QString mainfile) 
    : m_mainfile(mainfile)
    , m_editable(false) {}


void LocalFileTemplate::init(){

	if(readMetaData())
        return;

    const auto name = QFileInfo(file()).baseName();
	metaData.insert("Name",name);
}


QString LocalFileTemplate::imageFile() const {

	const auto path = replaceFileExtension(m_mainfile,"png");
	
	if(QFileInfo(path).exists())
        return path;

    return "";
}
