#ifndef Header_SearchResult_Widget
#define Header_SearchResult_Widget

#include "mostQtHeaders.h"
#include "SearchQuery.hpp"
#include "qdocumentsearch.h"
#include "qdocument.h"


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


class SearchTreeDelegate : public QItemDelegate {

	Q_OBJECT

	private:

		using Option = const QStyleOptionViewItem &;
		using Index = const QModelIndex &;
		using Painter = QPainter *;

	public:

		SearchTreeDelegate(QString editorFontFamily,QObject * parent = nullptr);

	protected:

		void paint(Painter,Option,Index) const;
		QSize sizeHint(Option,Index) const;

	private:

		QString m_editorFontFamily;

};


#endif
