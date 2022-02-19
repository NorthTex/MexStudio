#ifndef Header_Search_ResultWidget
#define Header_Search_ResultWidget


#include <QWidget>
#include <qdocument.h>

#include "SearchQuery.hpp"


class SearchResultWidget : public QWidget {

	Q_OBJECT

	public:

		explicit SearchResultWidget(QWidget * parent = nullptr);

		void setQuery(SearchQuery *);
		SearchQuery::Scope searchScope() const;
		void updateSearchExpr(QString searchText);

		void saveConfig();

	signals:

		void jumpToSearchResult(QDocument *,int lineNumber,const SearchQuery *);
		void runSearch(SearchQuery *);

	public slots:

		void clearSearch();

	private slots:

		void clickedSearchResult(const QModelIndex &);
		void updateSearch();
		void searchCompleted();

	private:

		QPushButton * searchAgainButton;
		QPushButton * replaceButton;
		QLineEdit * replaceTextEdit;
		QComboBox * searchScopeBox;
		QLabel * searchTypeLabel;
		QLabel * searchTextLabel;
		QTreeView * searchTree;

		SearchQuery * query;

		void retranslateUi();
		void updateSearchScopeBox(SearchQuery::Scope);

};


#endif
