#ifndef Header_MiktexPackageScanner
#define Header_MiktexPackageScanner


#include "PackageScanner.hpp"
#include <QString>
#include <QObject>


class MiktexPackageScanner : public PackageScanner {

	Q_OBJECT

	public:

		MiktexPackageScanner(QString mpmCmd,QString settingsDir,QObject * parent = 0);

	protected:

		void run();
		QString mpm(const QString & arg);
		void savePackageMap(const QHash<QString, QStringList> & map);
		QHash<QString, QStringList> loadPackageMap();
		QStringList stysForPackage(const QString & pck);

	private:

		QString mpmCmd;
		QString settingsDir;

};


#endif
