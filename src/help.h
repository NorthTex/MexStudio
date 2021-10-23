#ifndef Header_Help1
#define Header_Help1


#include "mostQtHeaders.h"
//#include "texdocdialog.h"
//#include "configmanager.h"

//#include "utilsSystem.h"
//#include "smallUsefulFunctions.h"
//#include <QProcessEnvironment>
//#include <QMutex>



//class Help : public QObject {

//	Q_OBJECT

//	private:

//	private:

//		QString runTexdoc(QString args);


//	public:

//		explicit Help(QObject * parent = nullptr);

//		bool isMiktexTexdoc();
//		bool isTexdocExpectedToFinish();
//		QString packageDocFile(const QString & package,bool silent = false);

//	signals:

//		void texdocAvailableReply(const QString & package,bool available,QString errorMessage);
//		void runCommandAsync(const QString & commandline,const char * returnCmd);
//		void statusMessage(const QString & message);
//		void runCommand(const QString & commandline,QString * output);

//	public slots:

//		void texdocAvailableRequestFinished(int,QProcess::ExitStatus);
//		void texdocAvailableRequest(const QString & package);
//		void execTexdocDialog(const QStringList & packages,const QString & defaultPackage);
//		void viewTexdoc(QString package);

//	private:

//		QString runTexdoc(QString args);

//		bool runTexdocAsync(QString args,const char * finishedCMD);

//		int texDocSystem;

//};


//class LatexReference : public QObject {

//	Q_OBJECT

//	private:

//		using Command = const QString &;

//	public:

//		explicit LatexReference(QObject * parent = 0);

//		bool contains(Command);
//		void setFile(QString filename);

//		QString getTextForTooltip(Command);
//		QString getSectionText(Command);
//		QString getPartialText(Command);

//	protected:

//		void makeIndex();

//	private:

//		struct Anchor {

//			Anchor()
//				: name(QString())
//				, start_pos(-1)
//				, end_pos(-1) {}

//			Anchor(const QString & name)
//				: name(name)
//				, start_pos(-1)
//				, end_pos(-1) {}

//			QString name;
//			int start_pos;
//			int end_pos;

//		};

//		QString m_filename;
//		QString m_htmltext;

//		QHash<QString, Anchor> m_anchors; // maps commands to their anchor
//		QHash<QString, Anchor> m_sectionAnchors; // maps commands to the anchor of their section

//};


#endif
