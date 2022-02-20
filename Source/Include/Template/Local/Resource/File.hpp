#ifndef Header_Template_Local_Resource_File
#define Header_Template_Local_Resource_File


#include "Template/AbstractResource.hpp"
#include "Template/Handle.hpp"
#include "Template/Local/File.hpp"

#include <QIcon>
#include <QString>
#include <QList>
#include <QObject>


class LocalFileTemplateResource : public QObject , public AbstractTemplateResource {

	Q_OBJECT
	Q_INTERFACES(AbstractTemplateResource)

    private:

		QList<LocalFileTemplate *> m_templates;

		QStringList m_filters;
		QIcon m_icon;

		QString 
            m_description,
            m_path,
            m_name;

    protected:

		LocalFileTemplateResource(QString path,QStringList filters,QString name,QObject * parent = 0,QIcon icon = QIcon());
		
        virtual LocalFileTemplate * createTemplate(QString file) = 0;
		
        void update();

	public:

		~LocalFileTemplateResource();

		virtual QList<TemplateHandle> getTemplates();
		virtual bool isAccessible();

    public:
		
        virtual QString name(){
            return m_name;
        }
		
        virtual QString description(){
            return m_description;
        }
		
        virtual QIcon icon(){
            return m_icon;
        }

    public:

		void setDescription(const QString & description){
            m_description = description;
        }

		void setEditable(bool);
};


#endif
