#ifndef Header_SearchResult_Model
#define Header_SearchResult_Model


#include "mostQtHeaders.h"


class QDocument;
class QDocumentLine;
class QDocumentLineHandle;


struct SearchInfo {

	QPointer<QDocument> doc;
	QList<QDocumentLineHandle *> lines;
	QList<bool> checked;

	mutable QList<int> lineNumberHints;

};


struct SearchMatch {

	int pos;
	int length;

};


class SearchResultModel : public QAbstractItemModel {

	Q_OBJECT

	private:

		using Index = const QModelIndex &;

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

		int getNextSearchResultColumn(const QString & text,int col);

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

	private:

		QVariant dataForResultEntry(const SearchInfo &,int lineIndex,int role) const;
		QVariant dataForSearchResult(const SearchInfo &,int role) const;
		QString prepareReplacedText(const QDocumentLine &) const;

		QList< SearchInfo > m_searches;
		QString mExpression, mReplacementText;
		QFont mLineFont;

		bool mIsWord , mIsCaseSensitive , mIsRegExp;
		bool mAllowPartialSelection;

};


Q_DECLARE_METATYPE(SearchMatch);
Q_DECLARE_METATYPE(QList<SearchMatch>);


class LabelSearchResultModel : public SearchResultModel {

	Q_OBJECT

	public:

		LabelSearchResultModel(QObject * parent = 0);

		QList<SearchMatch> getSearchMatches(const QDocumentLine &) const;

};


#endif
