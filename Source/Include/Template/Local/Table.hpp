#ifndef Header_Template_Local_Table
#define Header_Template_Local_Table


#include "Template/Local/File.hpp"


class LocalTableTemplate : public LocalFileTemplate {

	private:

		LocalTableTemplate(QString mainfile)
			: LocalFileTemplate(mainfile){

			init();
		}

	protected:

		virtual bool readMetaData();

    public:

		friend class LocalTableTemplateResource;
};


#endif
