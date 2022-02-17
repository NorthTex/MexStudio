#ifndef Header_LabelSearchQuery
#define Header_LabelSearchQuery


#include "SearchQuery.hpp"


class LabelSearchQuery : public SearchQuery {

	Q_OBJECT

	public:

		LabelSearchQuery(QString label);

		virtual void run(LatexDocument *);
		virtual void replaceAll();
};


#endif
