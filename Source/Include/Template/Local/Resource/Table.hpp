#ifndef Header_Template_Local_Resource_Table
#define Header_Template_Local_Resource_Table


#include "Template/Local/Resource/File.hpp"
#include "Template/Local/Table.hpp"


class LocalTableTemplateResource : public LocalFileTemplateResource {

	Q_OBJECT

    protected:

		virtual LocalFileTemplate * createTemplate(QString file){
			return new LocalTableTemplate(file);
		}

	public:

		LocalTableTemplateResource(
            QString path,
            QString name,
            QObject * parent,
            QIcon icon = QIcon()
        ) : LocalFileTemplateResource(path,
            QStringList() << "*.js",
            name,parent,icon){

			update();
		}
};


#endif
