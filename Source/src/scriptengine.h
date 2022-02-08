#ifndef Header_ScriptEngine
#define Header_ScriptEngine


#include "mostQtHeaders.h"
#include <QJSEngine>
#include <QJSValueIterator>
#include <QQmlEngine>
#include "qeditor.h"


class BuildManager;
class Texstudio;
class ProcessX;
class LatexEditorView;
class Macro;


class scriptengine : public QObject {

	Q_OBJECT

	public:

		scriptengine(QObject * parent = nullptr);
		~scriptengine();

		void run(const bool quiet = false);
		void setScript(const QString & script,bool allowWrite = false);
		void setEditorView(LatexEditorView *);

		static BuildManager * buildManager;
		static Texstudio * app;

		QStringList triggerMatches;
		int triggerId;

		static QList<Macro> * macros;

	protected slots:

		QJSValue replaceSelectedText(QJSValue replacementText,QJSValue options = QJSValue());
		QJSValue searchFunction(QJSValue searchFor,QJSValue arg1 = QJSValue(),QJSValue arg2 = QJSValue(),QJSValue arg3 = QJSValue());
		QJSValue replaceFunction(QJSValue searchFor,QJSValue arg1 = QJSValue(),QJSValue arg2 = QJSValue(),QJSValue arg3 = QJSValue());

		void insertSnippet(const QString & arg);
		void information(const QString & message);
		void critical(const QString & message);
		void warning(const QString & message);
		void alert(const QString & message);
		void debug(const QString & message);

		bool confirmWarning(const QString & message);
		bool confirm(const QString & message);

		#ifndef QT_NO_DEBUG
			void crash_assert();
		#endif

		void crash_sigsegv();
		void crash_sigfpe();
		void crash_stack();
		void crash_loop();
		void crash_throw();

		ProcessX * system(const QString & commandline,const QString & workingDirectory = QString());

		QVariant readFile(const QString & filename);
		QVariant getPersistent(const QString &name);

		bool setTimeout(const QString &fun,const int timeout);
		bool hasPersistent(const QString & name);

		void writeFile(const QString & filename,const QString & content);
		void setPersistent(const QString & name,const QVariant & value);
		void registerAsBackgroundScript(const QString & name = "");
		void save(const QString fn = "");
		void saveCopy(const QString & fileName);
		void runTimed(const QString fun);

	protected:

		QByteArray getScriptHash();
		void registerAllowedWrite();

		bool hasReadPrivileges();
		bool hasWritePrivileges();

		bool needReadPrivileges(const QString & fn, const QString & param);
		bool needWritePrivileges(const QString & fn, const QString & param);

		QJSEngine * engine;

		QJSValue searchReplaceFunction(QJSValue searchText,QJSValue,QJSValue,QJSValue,bool replace);

		QPointer<LatexEditorView> m_editorView;
		QPointer<QEditor> m_editor;
		QString m_script;

		bool m_allowWrite;

		static int writeSecurityMode , readSecurityMode;
		static QStringList privilegedReadScripts , privilegedWriteScripts;

};


#include "universalinputdialog.h"


class UniversalInputDialogScript: public UniversalInputDialog {

    Q_OBJECT

    public:

        Q_INVOKABLE UniversalInputDialogScript(QWidget * parent = nullptr);
        ~UniversalInputDialogScript();

    public slots:

        QWidget * add(const QJSValue & def,const QJSValue & description,const QJSValue & id = QJSValue());

        QJSValue execDialog();

        QJSValue getAll();
        QVariant get(const QJSValue & id);

    private:

        QJSEngine * engine;

};


#endif
