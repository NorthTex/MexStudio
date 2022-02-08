#ifndef Header_SearchQuery
#define Header_SearchQuery


#include "mostQtHeaders.h"
#include "searchresultmodel.h"
#include "qdocument.h"
#include "qdocumentline_p.h"


class LatexDocument;


class SearchQuery : public QObject {

	Q_OBJECT

	private:

		using Expression = QString;

	public:

		enum SearchFlag {
			NoFlags				= 0x0000,
			SearchParams		= 0x00FF,
			IsCaseSensitive		= 0x0001,
			IsWord				= 0x0002,
			IsRegExp			= 0x0004,

			SearchCapabilities	= 0xFF00,
			ScopeChangeAllowed	= 0x0100,
			ReplaceAllowed		= 0x0200,
			SearchAgainAllowed	= 0x0400,
		};


		Q_DECLARE_FLAGS(SearchFlags, SearchFlag)


		enum Scope {
			CurrentDocumentScope,
			ProjectScope,
			GlobalScope
		};


		SearchQuery(Expression,QString replaceText,SearchFlags);
		SearchQuery(Expression,QString replaceText,bool isCaseSensitive,bool isWord,bool isRegExp);

		bool flag(SearchFlag) const;

		QString type(){
			return mType;
		}

		QString searchExpression() const {
			return mModel -> searchExpression();
		}

		SearchResultModel * model() const {
			return mModel;
		}

		void setScope(Scope scope){
			mScope = scope;
		}

		Scope scope(){
			return mScope;
		}

		void setExpression(Expression);
		int getNextSearchResultColumn(QString text,int col) const;


	signals:

		void runCompleted();

	public slots:

		virtual void run(LatexDocument *);
		virtual void replaceAll();

		void addDocSearchResult(QDocument *,QList<QDocumentLineHandle *> search);
		void setReplacementText(QString text);

		QString replacementText();

	protected:

		void setFlag(SearchFlag,bool state = true);

		QString mType;
		Scope mScope;
		SearchResultModel * mModel;
		SearchFlags searchFlags;

};


Q_DECLARE_OPERATORS_FOR_FLAGS(SearchQuery::SearchFlags)


class LabelSearchQuery : public SearchQuery {

	Q_OBJECT

	public:

		LabelSearchQuery(QString label);

		virtual void run(LatexDocument *);
		virtual void replaceAll();

};


#endif
