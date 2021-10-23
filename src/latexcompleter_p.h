#ifndef Header_Latex_Completer_P
#define Header_Latex_Completer_P


#include "mostQtHeaders.h"
#include "codesnippet.h"
#include "latexcompleter_config.h"
#include <set>


using CompletionWord = CodeSnippet;


class CompletionListModel : public QAbstractListModel {

	Q_OBJECT

	public:

		CompletionListModel(QObject * parent = 0)
			: QAbstractListModel(parent)
			, mostUsedUpdated(false)
			, mCanFetchMore(false)
			, mLastMU(0)
			, mLastType(CodeSnippet::none)
			, mEnvMode(false)
			, mWordCount(0)
			, mCitCount(-1) {}

		int rowCount(const QModelIndex & parent = QModelIndex()) const;

		QVariant headerData(int section,Qt::Orientation,int role = Qt::DisplayRole) const;
		QVariant data(const QModelIndex &,int role)const;

		const QList<CompletionWord> & getWords(){
			return words;
		}

		bool isNextCharPossible(const QChar & c); //does this character lead to a new possible word
		void setContextWords(const QSet<QString> &newwords, const QString &context);
		void setEnvironMode(bool mode);
		void setKeyValWords(const QString &name, const QSet<QString> &newwords);
		void setAbbrevWords(const QList<CompletionWord> & newwords);
		void setBaseWords(const QSet<QString> & newwords,CompletionType);
		void setBaseWords(const std::set<QString> & newwords,CompletionType);
		void setBaseWords(const QList<CompletionWord> & newwords,CompletionType);
		void setBaseWords(const CodeSnippetList & baseCommands,const CodeSnippetList & newwords,CompletionType);
		void filterList(const QString & word,int mostUsed = -1,bool fetchMore = false,CodeSnippet::Type = CodeSnippet::none);
		void setConfig(LatexCompleterConfig *);
		void fetchMore(const QModelIndex & parent);
		void incUsage(const QModelIndex &);

		virtual bool canFetchMore(const QModelIndex & parent) const;


		CompletionWord getLastWord();

	private:

		friend class LatexCompleter; //TODO: make this unnecessary

		QList<CompletionWord> words;
		QString curWord;

		QList<CompletionWord> baselist;
		QList<CompletionWord> wordsText , wordsCommands , wordsAbbrev , wordsLabels , wordsCitations;

		int mostUsedUpdated;

		QMap<QString, QList<CompletionWord> > keyValLists;
		QMap<QString, QList<CompletionWord> > contextLists;

		bool mCanFetchMore;
		QString mLastWord;
		int mLastMU;
		CodeSnippet::Type mLastType;
		CompletionWord mLastWordInList;

		bool mEnvMode;

		int mWordCount, mCitCount;

		QList<CompletionWord>::iterator it;

		static LatexCompleterConfig * config;

};

#endif

