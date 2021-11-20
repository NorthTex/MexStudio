#ifndef QT_NO_DEBUG
#include "qdocumentline_t.h"

//Actually this is a qdocumentlinehandle test, but they are almost the same

//----
//force full access to qdocument things
#define private public
#include "qdocument_p.h"
#include "qdocumentline_p.h"
#undef private
//----

#include "qeditor.h"
#include "qdocumentline.h"
#include "qdocumentline_p.h"
#include "testutil.h"
#include "smallUsefulFunctions.h"
#include <QtTest/QtTest>

using QTest::addColumn;
using QTest::addRow;


Test::DocumentLine::DocumentLine()
	: savedFixedPitch(false)
	, savedSpaceWidth(0)
	, savedLeftPadding(0)
	, savedWorkAroundFPM(false)
	, savedWorkAroundFSCD(false)
	, savedWorkAroundFQTL(false){

	doc = new QDocument(this);
}

Test::DocumentLine::~DocumentLine(){}

void Test::DocumentLine::initTestCase(){

	savedFixedPitch = QDocumentPrivate::m_fixedPitch;
	savedSpaceWidth = QDocumentPrivate::m_spaceWidth;
	savedLeftPadding = QDocumentPrivate::m_leftPadding;

	savedWorkAroundFPM = doc -> hasWorkAround(QDocument::DisableFixedPitchMode);
	savedWorkAroundFSCD = doc -> hasWorkAround(QDocument::ForceSingleCharacterDrawing);
	savedWorkAroundFQTL = doc -> hasWorkAround(QDocument::ForceQTextLayout);
	
	doc -> impl() -> setHardLineWrap(false);
	doc -> setWorkAround(QDocument::DisableFixedPitchMode,false);
	doc -> setWorkAround(QDocument::ForceSingleCharacterDrawing,false);
	doc -> setWorkAround(QDocument::ForceQTextLayout,false);

	QDocumentPrivate::m_fixedPitch = true;
	QDocumentPrivate::m_leftPadding = 0;
	QDocumentPrivate::m_spaceWidth = 5;
}

void Test::DocumentLine::cleanupTestCase(){
	
	QDocumentPrivate::m_fixedPitch = savedFixedPitch;
	QDocumentPrivate::m_spaceWidth = savedSpaceWidth;
	QDocumentPrivate::m_leftPadding = savedLeftPadding;
	
	doc -> setWorkAround(QDocument::DisableFixedPitchMode,savedWorkAroundFPM);
	doc -> setWorkAround(QDocument::ForceSingleCharacterDrawing,savedWorkAroundFSCD);
	doc -> setWorkAround(QDocument::ForceQTextLayout,savedWorkAroundFQTL);
}

using IntPair = QPair<int,int>;

void Test::DocumentLine::updateWrap_data(){

	addColumn<QString>("line");
	addColumn<int>("width");

	//every letter = 5px

	addRow("no wrap") << "abc" << 20;
	addRow("no wrap - exact hit") << "abcd" << 20;
	addRow("one pixel off") << "abc|d" << 19;
	addRow("3 lines force-break") << "abcd|efgh|ijkl" << 20;
	addRow("3 lines force-break - 2px waste") << "abcd|efgh|ijkl" << 22;
	addRow("3 lines force-break - 3px waste") << "abcd|efgh|ijkl" << 23;
	addRow("4 lines force-break - 2px off") << "abc|def|ghi|jkl" << 18;
	addRow("word break") << "ab |cde" << 20;
	addRow("word break - 2 spaces") << "ab  |cde" << 20;//??
	addRow("word break - 3 spaces nowhere to go") << "ab  | cde" << 20;
	addRow("word break - 4 spaces nowhere to go") << "ab  |  cd" << 20;
	addRow("word break - 4 spaces nowhere to go") << "ab  |  |cde" << 20;
	addRow("word break - space bug") << "a |a  x" << 20;
	addRow("space break") << "x   |    |    |   " << 20;
	addRow("space break - 1px off") << "x  |   |   |   " << 19;
	addRow("indentation break 1") << " abc|def|ghi" << 20;
	addRow("indentation break 2") << "  ab|cd|ef" << 20;
	addRow("indentation break 2 - word") << "  ax |bcd" << 30;
	addRow("indentation breaking") << "    |    |    |   " << 20;
	addRow("indentation breaking cat") << "    |    |    |   |miau" << 20;
	addRow("indentation breaking cat in 3d") << "    |    |    |   |miau|xyz" << 20;
}

void Test::DocumentLine::updateWrap(){

	QFETCH(QString,line);
	QFETCH(int,width);

	QString temp = line;
	temp.replace('|',"");

	doc -> setText(temp + "\n\n\n",false);
	doc -> setWidthConstraint(width);

	QString fExp, fGot;
	QList<int> frontiers;
	
	for(int i = 0;i < line.size();i++)
		if(line.at(i) == '|')
			frontiers << (i - frontiers.size());

	for(const auto p : frontiers)
		fExp += QString("<%1:%2>").arg(p).arg(p * 5);

	auto m_frontiers = doc -> line(0).handle() -> m_frontiers;

	for(const auto & p : m_frontiers)
		fGot += QString("<%1:%2>").arg(p.first).arg(p.second);


	QEQUAL(fGot,fExp);

	
	//-------test hard line wrap----
	QList<QDocumentLineHandle*> handles;

	for(int i = 0;i < doc -> lines();i++)
		handles << doc -> line(i).handle();
	
	doc -> applyHardLineWrap(handles);
	
	QStringList hlw = line.split('|');
	QString indent;

	auto first = hlw.first();

	for(auto c : first){

		if(!c.isSpace())
			break;

		indent += c;
	}


	// for(int i = 0;i < first.length();i++){

	// 	if(first[i].isSpace()){
	// 		indent += first[i];
	// 		continue;
	// 	}

	// 	break;
	// }
	
	if(indent == first)
		indent = "";
	
	for(int i = 1;i < hlw.size();i++)
		hlw[i] = indent + hlw[i];
	
	hlw << "" << "" << "";
	
	QEQUAL(doc -> text(),hlw.join("\n"));
}

#endif
