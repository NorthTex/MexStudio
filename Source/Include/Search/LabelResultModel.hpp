#ifndef Header_Search_LabelResultModel
#define Header_Search_LabelResultModel


#include "Search/ResultModel.hpp"


class LabelSearchResultModel : public SearchResultModel {

	Q_OBJECT

	public:

		LabelSearchResultModel(QObject * parent = 0);

	public:

		QList<SearchMatch> getSearchMatches(const QDocumentLine &) const;

};


#endif
