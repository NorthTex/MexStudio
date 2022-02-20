#ifndef Header_Template_AbstractResource
#define Header_Template_AbstractResource


#include "Template/Handle.hpp"


class AbstractTemplateResource {

    protected:

		AbstractTemplateResource(){}

	public:

		virtual QList<TemplateHandle> getTemplates() = 0;

		virtual QString description() = 0;
		virtual QString name() = 0;

		virtual bool isAccessible() = 0;

		virtual QIcon icon() = 0;
};


Q_DECLARE_INTERFACE(AbstractTemplateResource,"TeXstudio/AbstractTemplateResource")
Q_DECLARE_METATYPE(AbstractTemplateResource *)


#endif
