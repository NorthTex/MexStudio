#ifndef Header_Template_LocalLatexResource
#define Header_Template_LocalLatexResource


#include "Template/Local/Resource/File.hpp"
#include "Template/Local/Latex.hpp"


class LocalLatexTemplateResource : public LocalFileTemplateResource {

	Q_OBJECT

    protected:

		virtual LocalFileTemplate * createTemplate(QString file){
			return new LocalLatexTemplate(file);
		}

	public:

		LocalLatexTemplateResource(
            QString path,
            QString name,
            QObject * parent,
            QIcon icon = QIcon()
        ) : LocalFileTemplateResource(path,
            QStringList() << "*.tex" << "*.lytex" << "*.zip" << "*.ctx",
            name,parent,icon) {

			update();
		}
};


#endif
