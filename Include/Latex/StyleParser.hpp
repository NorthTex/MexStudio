#ifndef Header_Latex_StyleParser
#define Header_Latex_StyleParser


#include "mostQtHeaders.h"
#include "utilsSystem.h"
#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QQueue>


class LatexStyleParser : public SafeThread {

	Q_OBJECT

	#ifndef QT_NO_DEBUG
		friend class LatexStyleParserTest;
	#endif

	public:

		explicit LatexStyleParser(QObject * parent = 0,QString baseDirName = "",QString kpsecmd = "");

		void stop();
		void addFile(QString filename);
		void setAlias(QMultiHash<QString,QString> PackageAliases){
			mPackageAliases = PackageAliases;
		}

	protected:

		void run();

	private:

		struct XpArg {

			bool optional;
			QChar delimLeft;
			QChar delimRight;
			QChar fixedChar;

		};

		using XpArgList = QList<XpArg>;
		using Results = QStringList &;
		using Line = const QString &;

		bool parseLineInput(QStringList &results, const QString &line, QStringList &parsedPackages, const QString &fileName) const;
		bool parseLineLoadClass(QStringList &results, const QString &line) const;

		QStringList parseLine(const QString &line, bool &inRequirePackage, QStringList &parsedPackages, const QString &fileName) const;
		QStringList readPackage(QString fileName, QStringList &parsedPackages) const;
		QStringList readPackageTexDef(QString fn) const;
		QStringList readPackageTracing(QString fn) const;
		QString kpsewhich(QString name, QString dirName = "") const;

		static QString makeArgString(int count, bool withOptional=false);

		static void parseLineXparseOutputCwl(Results,const QString & prefix,XpArgList::const_iterator itPos,XpArgList::const_iterator itEnd,int argIndex);
		static void parseLineXparseOutputCwl(Results,const QString & prefix,const XpArgList &xpArgs,int index);

		static bool parseLineRequirePackage(Results,Line,bool & inRequirePackage);
		static bool parseLineXparseCommand(Results,Line);
		static bool parseLineXparseOneArg(XpArg & xpArg,const QString & argDef);
		static bool parseLineRequireStart(Results,Line,bool & inRequirePackage);
		static bool parseLineXparseArgs(XpArgList & xpArgs,Line,int lineOffset);
		static bool parseLineNewCounter(Results,Line);
		static bool parseLineDecMathSym(Results,Line);
		static bool parseLineXparseEnv(Results,Line);
		static bool parseLineNewLength(Results,Line);
		static bool parseLineGetGroup(QString & group,Line,int groupStart);
		static bool parseLineRequire(Results,Line);
		static bool parseLineCommand(Results,Line);
		static bool parseLineEnv(Results,Line);
		static bool parseLineLet(Results,Line);
		static bool parseLineDef(Results,Line);

	signals:

		void scanCompleted(QString package);

	private:

		QQueue<QString> mFiles;
		QSemaphore mFilesAvailable;
		QMutex mFilesLock;

		bool stopped;

		QString baseDir;
		QString kpseWhichCmd;
		QString texdefDir;

		bool texdefMode;

		QMultiHash<QString, QString> mPackageAliases;

};


#endif
