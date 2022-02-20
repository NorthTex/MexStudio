
#include "Template/Local/Table.hpp"
#include "Template/Manager.hpp"
#include <QJsonDocument>
#include <Include/UtilsUI.hpp>


bool LocalTableTemplate::readMetaData(){

	QString json;
	QFile f(file());
	
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		UtilsUi::txsWarning(TemplateManager::tr("You do not have read permission to this file:") + QString("\n%1").arg(f.fileName()));
		return false;
	}
	
    json = f.readLine();
    
    
    int column = json.indexOf(QRegularExpression("\\s*var\\s+metaData\\s+=\\s+\\{"));
	
    if(column < 0)
        return false;

	json = json.mid(column);

	QString all = f.readAll();
    column = all.indexOf("\n}\n"); // simplified, search for }, first in line
    
    if(column >= 0)
        all = all.left(column);

    json = "{\n" + all + "\n}\n";


    const auto document = QJsonDocument::fromJson(json.toUtf8());
    const auto data = document.object();

    for(const auto & key : data.keys())
        if(data[key].isString())
            metaData.insert(key,data[key].toString());

    return true;
}

