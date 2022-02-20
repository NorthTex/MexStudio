#ifndef Header_Search_Info
#define Header_Search_Info


#include <QList>
#include <qdocument.h>
#include <qdocumentline.h>


struct SearchInfo {

	QPointer<QDocument> doc;

	QList<QDocumentLineHandle *> lines;
	QList<bool> checked;

	mutable QList<int> lineNumberHints;

};


#endif
