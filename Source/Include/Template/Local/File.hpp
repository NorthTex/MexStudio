#ifndef Header_Template_Local_File
#define Header_Template_Local_File


#include "Template/Template.hpp"
#include <QString>
#include <QHash>
#include <QDate>
#include <QPixmap>


class LocalFileTemplate : public Template {

	private:

		// a single .tex file or a .zip for multiple files

		QString m_mainfile;
		bool m_editable;

	protected:

		QHash<QString,QString> metaData;

	private:

		QString imageFile() const;

	public:

		friend class LocalFileTemplateResource;

	protected:

		LocalFileTemplate(QString mainfile);
		
		void init();

	protected:

		virtual bool readMetaData(){
			return false;
		}

		virtual bool saveMetaData(){
			return false;
		}

	public:
		
		virtual QString name() const {
			return metaData["Name"];
		}
		
		virtual QString description() const {
			return metaData["Description"];
		}

		virtual QString author() const {
			return metaData["Author"];
		}

		virtual QString version() const {
			return metaData["Version"];
		}
		
		virtual QString license() const {
			return metaData["License"];
		}

		virtual QPixmap previewImage() const {
			return QPixmap(imageFile());
		}

		virtual bool isEditable() const {
			return m_editable;
		}
		
		virtual QString file() const {
			return m_mainfile;
		}

		virtual QStringList filesToOpen() const;
		virtual QDate date() const;
};


#endif
