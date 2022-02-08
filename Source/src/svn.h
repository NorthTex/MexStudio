#ifndef Header_SVN
#define Header_SVN


#include "mostQtHeaders.h"


/*!
 * \brief SVN class
 * This class provides easy access to the svn command.
 */

class SVN : public QObject {

	Q_OBJECT

	private:

		using Path = QString;
		using Action = QString;
		using Args = QString;

	public:

		enum Status {Unknown, Unmanaged, Modified, Locked, CheckedIn, InConflict};

		explicit SVN(QObject * parent = Q_NULLPTR);

		static QString quote(Path);
		static QString makeCmd(Action,Args);
		static QString makeAdminCmd(Action,Args);

		void commit(Path,QString message);
		void lock(Path);
		void createRepository(Path);

		Status status(Path);
		QStringList log(Path);

		QString runSvn(Action,Args);
		QString runSvnAdmin(Action,Args);

	signals:

		void runCommand(const QString & commandline,QString * output);
		void statusMessage(const QString & message);

};

#endif
