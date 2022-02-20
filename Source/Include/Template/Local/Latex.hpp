#ifndef Header_Template_Local_Latex
#define Header_Template_Local_Latex


#include "Template/Local/File.hpp"


class LocalLatexTemplate : public LocalFileTemplate {

    private:

		LocalLatexTemplate(QString mainfile)
			: LocalFileTemplate(mainfile) {

			init();
		}

    protected:

		virtual bool readMetaData();
		virtual bool saveMetaData();

	public:

		friend class LocalLatexTemplateResource;
};


#endif
