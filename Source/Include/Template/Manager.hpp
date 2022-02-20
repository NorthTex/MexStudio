#ifndef Header_Template_Manager
#define Header_Template_Manager


#include "Template/Handle.hpp"
#include "Template/AbstractResource.hpp"
#include "templateselector.h"

#include <QObject>
#include <QString>
#include <QDomElement>


class TemplateManager : public QObject {

	Q_OBJECT
    
    private:

        static QString configBaseDir;
        QString selectedFile;

    private slots:

        void editTemplate(TemplateHandle);
        void editTemplateInfo(TemplateHandle);

    signals:

        void editRequested(const QString & filename);

    public:

        static void setConfigBaseDir(const QString & dir){
            configBaseDir = dir;
        }

        static QString userTemplateDir(){
            return configBaseDir + "templates/user/";
        }

        static bool ensureUserTemplateDirExists();
        static void checkForOldUserTemplates();
        static QString builtinTemplateDir();

    public:

		explicit TemplateManager(QObject * parent = nullptr);

        AbstractTemplateResource * createResourceFromXMLNode(const QDomElement & node);
        QList<AbstractTemplateResource *> resourcesFromXMLFile(const QString & path);

        TemplateSelector * createLatexTemplateDialog();

        bool tableTemplateDialogExec();

        QString selectedTemplateFile(){
            return selectedFile;
        }
};


#endif
