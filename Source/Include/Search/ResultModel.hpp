#ifndef Header_Search_ResultModel
#define Header_Search_ResultModel


#include <QAbstractItemModel>
#include "Search/Info.hpp"
#include <qdocument.h>
#include "Search/Match.hpp"


class SearchResultModel : public QAbstractItemModel {

	Q_OBJECT

	private:

		using Index = const QModelIndex &;

	private:

		QList< SearchInfo > m_searches;
		QString mExpression , mReplacementText;
		QFont mLineFont;

		bool mIsWord , mIsCaseSensitive , mIsRegExp;
		bool mAllowPartialSelection;

	private:

		QVariant dataForResultEntry(const SearchInfo &,int lineIndex,int role) const;
		QVariant dataForSearchResult(const SearchInfo &,int role) const;
		QString prepareReplacedText(const QDocumentLine &) const;

	public:

		enum UserRoles { LineNumberRole = Qt::UserRole , MatchesRole };

		SearchResultModel(QObject * parent = 0);
		~SearchResultModel();

		QModelIndex index(int row,int column,const QModelIndex & parent = QModelIndex()) const;
		QVariant headerData(int section,Qt::Orientation orientation,int role) const;

		QVariant data(Index,int role) const;
		QModelIndex parent(Index) const;
		Qt::ItemFlags flags(Index) const;

		QList<SearchInfo> getSearches();
		QDocument * getDocument(Index);

		bool setData(Index, const QVariant &value, int role);

		int getNextSearchResultColumn(const QString & text,int col);
		int rowCount(Index parent = QModelIndex()) const;
		int columnCount(Index) const;
		int getLineNumber(Index);

		void setSearchExpression(const QString & exp,const QString & repl,const bool isCaseSensitive,const bool isWord,const bool isRegExp);
		void setSearchExpression(const QString & exp,const bool isCaseSensitive,const bool isWord,const bool isRegExp);
		void addSearch(const SearchInfo &);
		void removeSearch(const QDocument *);
		void removeAllSearches();
		void clear();

		QString searchExpression(){
			return mExpression;
		}


		void getSearchConditions(bool & isCaseSensitive,bool & isWord,bool & isRegExp){
			isWord = mIsWord;
			isCaseSensitive = mIsCaseSensitive;
			isRegExp = mIsRegExp;
		}

		void setReplacementText(QString text){
			mReplacementText = text;
		}

		QString replacementText(){
			return mReplacementText;
		}

		void setAllowPartialSelection(bool b){
			mAllowPartialSelection = b;
		}

		virtual QList<SearchMatch> getSearchMatches(const QDocumentLine &) const;
};


Q_DECLARE_METATYPE(SearchMatch);
Q_DECLARE_METATYPE(QList<SearchMatch>);


#endif
