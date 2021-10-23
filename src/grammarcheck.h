#ifndef Header_GrammarCheck
#define Header_GrammarCheck


#include "mostQtHeaders.h"
#include "latexparser/latexparser.h"
#include "configmanagerinterface.h"


//TODO: move this away
#include "grammarcheck_config.h"
class QDocumentLineHandle;
class LatexDocument;


struct LineInfo {

    QDocumentLineHandle* line;
	QString text;

};


enum GrammarErrorType {

	GET_UNKNOWN = 0,
	GET_WORD_REPETITION,
	GET_LONG_RANGE_WORD_REPETITION,
	GET_BAD_WORD,
	GET_BACKEND,
	GET_BACKEND_SPECIAL1,
	GET_BACKEND_SPECIAL2,
	GET_BACKEND_SPECIAL3,
	GET_BACKEND_SPECIAL4
};


struct GrammarError {

	int offset;
	int length;

	GrammarErrorType error;
	QString message;
	QStringList corrections;

	GrammarError(int offset,int length,const GrammarErrorType & error,const QString & message,const QStringList & corrections);
	GrammarError(int offset,int length,const GrammarErrorType & error,const QString & message);
	GrammarError(int offset,int length,const GrammarError & other);
	GrammarError();

};


struct LineResult {

    QDocumentLineHandle * line;
    LatexDocument * doc;

    int lineNr;

    QList<GrammarError> errors;

};


Q_DECLARE_METATYPE(LineInfo)
Q_DECLARE_METATYPE(GrammarError)
Q_DECLARE_METATYPE(GrammarErrorType)
Q_DECLARE_METATYPE(QList<LineInfo>)
Q_DECLARE_METATYPE(QList<GrammarError>)


struct LanguageGrammarData {

	QSet<QString> stopWords;
	QSet<QString> badWords;

};


class GrammarCheckBackend;
struct CheckRequest;


using TicketHash = QHash<QDocumentLineHandle *,QPair<uint,int>>;


class GrammarCheck : public QObject {

	Q_OBJECT

	public:

		enum LTStatus { LTS_Unknown , LTS_Working , LTS_Error };

		explicit GrammarCheck(QObject * parent = nullptr);
		~GrammarCheck();

		LTStatus languageToolStatus(){
			return ltstatus;
		}

		QString getLastErrorMessage();
		QString serverUrl();

	signals:

		void checked(LatexDocument *,QDocumentLineHandle *,int lineNr,QList<GrammarError> errors);
		void languageToolStatusChanged();
		void errorMessage(QString message);

	public slots:

		void check(const QString & language,LatexDocument *,const QList<LineInfo> & lines,int firstLineNr);
		void init(const LatexParser &,const GrammarCheckerConfig &);

		void shutdown();

	private slots:

		void backendChecked(uint ticket,int subticket,const QList<GrammarError> & errors,bool directCall = false);
		void process(int reqId);

		void updateLTStatus();
		void processLoop();

	private:

		QString languageFromHunspellToLanguageTool(QString language);
		QString languageFromLanguageToolToHunspell(QString language);

		LTStatus ltstatus;
		LatexParser * latexParser;
		GrammarCheckerConfig config;
		GrammarCheckBackend * backend;
		QMap<QString, LanguageGrammarData> languages;

		uint ticket;

		bool pendingProcessing;
		bool shuttingDown;

		TicketHash tickets;
		QList<CheckRequest> requests;
		QSet<QString> floatingEnvs;
		QHash<QString,QString> languageMapping;

};


struct CheckRequestBackend;


class GrammarCheckBackend : public QObject {

	Q_OBJECT

	public:

		GrammarCheckBackend(QObject * parent);

		virtual void check(uint ticket,uint subticket,const QString & language,const QString & text) = 0;
		virtual void init(const GrammarCheckerConfig &) = 0;

		virtual bool isAvailable() = 0;
		virtual bool isWorking() = 0;
		virtual void shutdown() = 0;

		virtual QString getLastErrorMessage() = 0;
		virtual QString url() = 0;

	signals:

		void checked(uint ticket,int subticket,const QList<GrammarError> & errors);
		void errorMessage(QString message);

		void languageToolStatusChanged();

};


class QNetworkAccessManager;
class QNetworkReply;


class GrammarCheckLanguageToolJSON: public GrammarCheckBackend {

    Q_OBJECT

    public:

        GrammarCheckLanguageToolJSON(QObject *parent = nullptr);
        ~GrammarCheckLanguageToolJSON();

        virtual void check(uint ticket,uint subticket,const QString & language,const QString & text);
        virtual void init(const GrammarCheckerConfig &);

        virtual QString getLastErrorMessage();
        virtual QString url();

        virtual bool isAvailable();
        virtual bool isWorking();
        virtual void shutdown();

    private slots:

        void finished(QNetworkReply * reply);

    private:

        QNetworkAccessManager * nam;
        QUrl server;

        enum Availability { Terminated , Broken , Unknown , WorkedAtLeastOnce };

        Availability connectionAvailability;

        bool triedToStart;
        bool firstRequest;

        QPointer<QProcess> javaProcess;

        QString ltPath, javaPath, ltArguments;
        QSet<QString> ignoredRules;
        QList<QSet<QString> >  specialRules;
        uint startTime;

        void tryToStart();

        QList<CheckRequestBackend> delayedRequests;

        QSet<QString> languagesCodesFail;
        QString errorText; // last error message

};


#endif
