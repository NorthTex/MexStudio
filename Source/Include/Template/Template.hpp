#ifndef Header_Template
#define Header_Template


#include "Template/Handle.hpp"
#include <QList>
#include <QString>
#include <QDate>
#include <QPixmap>


class TemplateHandle;


class Template {

    private:

		QList<TemplateHandle *> handles;

	public:

		virtual ~Template();

		#define getter(name,type) \
			virtual type name() const { return type(); }

		getter(         name , QString     )
		getter(  description , QString     )
		getter(       author , QString     )
		getter(      version , QString     )
		getter(      license , QString     )
		getter(         file , QString     )
		getter(         date , QDate       )
		getter( previewImage , QPixmap     )
		getter(  filesToOpen , QStringList )

		#undef getter

		virtual bool isEditable() const {
			return false;
		}

		virtual bool isMultifile() const {
			return file().endsWith(".zip");
		}

    public:

        bool createInFolder(const QString & path);

		void ref(TemplateHandle * handle){
			handles.append(handle);
		}

		void deref(TemplateHandle * handle){
			handles.removeOne(handle);
        }
};


#endif
