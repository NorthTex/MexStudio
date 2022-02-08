
#include "templatemanager_p.h"
#include <utilsUI.h>
#include "smallUsefulFunctions.h"
#include <QJsonDocument>


bool LocalLatexTemplate::readMetaData(){

	QFile f(replaceFileExtension(file(),"json"));
	
    if(!f.exists()){
	
        // in a very early version of meta data .meta was used instead of .json

    	f.setFileName(replaceFileExtension(file(),"meta"));
	
    	if(!f.exists())
			return false;
	}

	if(!f.open(QIODevice::ReadOnly | QIODevice::Text)){
		UtilsUi::txsWarning(TemplateManager::tr("You do not have read permission to this file:") + QString("\n%1").arg(f.fileName()));
		return false;
	}

	QTextStream stream(& f);

	const auto document = QJsonDocument::fromJson(stream.readAll().toUtf8());
	const auto json = document.object();
    
    for(const auto & key : json.keys())
        if(json[key].isString())
            metaData.insert(key,json[key].toString());
    
    return true;
}


bool LocalLatexTemplate::saveMetaData(){

	QFile f(file());

	if(!f.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

    QJsonObject json;
    QJsonDocument document(json);
	
    for(const auto & key : metaData.keys())
        json[key] = metaData[key];
    
    f.write(document.toJson());
	
    return true;
}
