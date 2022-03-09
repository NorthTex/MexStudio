#ifndef Header_PDF_MultiProcessX
#define Header_PDF_MultiProcessX


#include <QObject>
#include <QFileInfo>


class MultiProcessX: public QObject {

	Q_OBJECT

	protected:

		// cache commands and run them all at once, because 
        // we do not have access to the build manager here

		QList<QPair<QString,QFileInfo>> pendingCmds;
		QStringList temporaryFiles;

    protected:

		virtual ~MultiProcessX();

    protected:

		QString createTemporaryFileName(const QString & extension);

    protected:

		void run(const QString & cmd,const QFileInfo & master = QFileInfo());
		void execute();

	signals:

		void runCommand(
            const QString & command,
            const QFileInfo & masterFile,
            const QFileInfo & currentFile,
            int linenr
        );

};


#endif
