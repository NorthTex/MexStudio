#ifndef Header_KpathSeaParser
#define Header_KpathSeaParser


#include "PackageScanner.hpp"
#include <QObject>
#include <QString>


class KpathSeaParser : public PackageScanner {

	Q_OBJECT

	public:

		explicit KpathSeaParser(QString kpsecmd,QObject * parent = 0,QString additionalPaths = "");

	protected:

        void run();
        QString kpsewhich(const QString & arg);

    private:

        QString kpseWhichCmd;
        QString m_additionalPaths;
};


#endif
