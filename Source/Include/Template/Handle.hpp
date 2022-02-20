#ifndef Header_Template_Handle
#define Header_Template_Handle


#include "Template/Template.hpp"

#include <QString>
#include <QPixmap>
#include <QDate>


class Template;


class TemplateHandle {

    private:

		void setTmpl(Template *);
	
        // only write via setTmpl()

    	Template * m_tmpl;

	public:

		friend class Template;

    public:

        TemplateHandle(const TemplateHandle &);

		TemplateHandle() 
            : m_tmpl(nullptr) {}
		
		explicit TemplateHandle(Template *);
		
        ~TemplateHandle();

    public:

		TemplateHandle & operator = (const TemplateHandle &);

		QStringList filesToOpen() const;
		QPixmap previewImage() const;
		QDate date() const;

		QString description() const;
		QString license() const;
		QString version() const;
		QString author() const;
		QString name() const;
		QString file() const;

		bool createInFolder(const QString & path) const;
		bool isMultifile() const;
		bool isEditable() const;
		bool isValid() const;
};


Q_DECLARE_METATYPE(TemplateHandle)


#endif
