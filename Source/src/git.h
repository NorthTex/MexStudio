#ifndef Header_Git
#define Header_Git


#include "mostQtHeaders.h"


/*!
 * \brief GIT class
 * This class provides easy access to the git command.
 */

class GIT : public QObject {

	Q_OBJECT

	public:

		enum Status {
			Unknown,
			Unmanaged,
			Modified,
			Locked,
			CheckedIn,
			InConflict,
			NoRepository
		};

		explicit GIT(QObject * parent = Q_NULLPTR);

		static QString makeCmd(QString action,QString args);
		static QString quote(QString filename);

		void commit(QString filename,QString message);
		void createRepository(QString filename);

		QStringList log(QString filename);
		Status status(QString filename);

		QString runGit(QString action,QString args);
		QString runGit(QString action,QString path,QString args);

	signals:

		void statusMessage(const QString & message);
		void runCommand(const QString & commandline,QString * output);

};


#endif
