
#include "help_t.h"

using namespace QTest;

void Test::Help::packageDocFile_data(){
	addColumn<QString>("package");
	addColumn<QString>("fileWithoutPath");

	newRow("fancyhdr") << "fancyhdr" << "fancyhdr.dvi;fancyhdr.pdf";
	newRow("ifthen") << "ifthen" << "ifthen.pdf";
	newRow("graphicx") << "graphicx" << "graphicx.pdf;grfguide.pdf";
	newRow("fancyref") << "fancyref" << "freftest.dvi;fancyref.pdf";
}

void Test::Help::packageDocFile(){

	QFETCH(QString,package);
	QFETCH(QString,fileWithoutPath);

	if(!globalExecuteAllTests){
		qDebug("skip");
		return;
	}

	QStringList checkList = fileWithoutPath.split(';');
	QString texdocPathname = helper.packageDocFile(package,true);

	if(texdocPathname == "")
		QVERIFY2(false, QString("texdoc command was not found in the search path or package \"%1\" is not installed").arg(package).toLatin1().constData());

	QString texdocFilename = QFileInfo(texdocPathname).fileName();
	bool found = false;

	for(auto && filename : checkList)
		if(filename == texdocFilename){
			found = true;
			break;
		}

	QVERIFY2(found,QString("Could not find documentation file for package \"%1\"").arg(package).toLatin1().constData());
}
