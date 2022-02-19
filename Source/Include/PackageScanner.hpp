#ifndef Header_PackageScanner
#define Header_PackageScanner


#include "utilsSystem.h"
#include <set>


class PackageScanner : public SafeThread {

	Q_OBJECT

	public:

		static void savePackageList(std::set<QString> packages,const QString & filename);
		static std::set<QString> readPackageList(const QString & filename);

		void stop();

	signals:

		void scanCompleted(std::set<QString> packages);

	protected:

		explicit PackageScanner(QObject * parent = 0);
		bool stopped;

};


#endif
