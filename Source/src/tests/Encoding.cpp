#ifndef QT_NO_DEBUG

#include "tests/Encoding.hpp"
#include "Include/Encoding.hpp"


using QTest::addColumn;
using QTest::newRow;


void Test::Encoding::test_lineStart_data(){

	addColumn<QString>("text");
	addColumn<int>("pos");
	addColumn<int>("start");

	newRow("empty") << "" << 0 << 0;
	newRow("start") << "foo bar" << 0 << 0;
	newRow("text") << "foo bar" << 6 << 0;
	newRow("newline") << "foo\nbar" << 6 << 4;
	newRow("cr") << "foo\rbar" << 6 << 4;
	newRow("cr_newline") << "foo\r\nbar" << 6 << 5;
	newRow("start_newline") << "foo\nbar" << 4 << 4;
}

void Test::Encoding::test_lineStart(){

	using namespace Encoding;

	QFETCH(QString, text);
	QFETCH(int, pos);
	QFETCH(int, start);
	QEQUAL(Internal::lineStart(text.toLatin1(),pos),start);
}

void Test::Encoding::test_lineEnd_data(){

	addColumn<QString>("text");
	addColumn<int>("pos");
	addColumn<int>("end");

	newRow("empty") << "" << 0 << 0;
	newRow("end") << "foo bar" << 7 << 7;
	newRow("text") << "foo bar" << 1 << 7;
	newRow("newline") << "foo\nbar" << 1 << 3;
	newRow("cr") << "foo\rbar" << 1 << 3;
	newRow("cr_newline") << "foo\r\nbar" << 1 << 3;
	newRow("end_newline") << "foo\nbar" << 3 << 3;
	newRow("end_newline") << "foo\r\nbar\x20\x29more" << 2 << 3;
}

void Test::Encoding::test_lineEnd(){

	using namespace Encoding;

	QFETCH(QString,text);
	QFETCH(int,pos);
	QFETCH(int,end);
	QEQUAL(Internal::lineEnd(text.toLatin1(),pos),end);
}

void Test::Encoding::test_getEncodingFromPackage_data(){

	addColumn<QString>("text");
	addColumn<QString>("encodingName");

	newRow("empty") << "" << "";
	newRow("no info") << "foo bar\nbaz" << "";
	newRow("simple") << "\\usepackage[latin1]{inputenc}" << "latin1";
	newRow("simple2") << "foo bar\nbaz\n\\usepackage[latin1]{inputenc}" << "latin1";
	newRow("spaces") << "  \\usepackage[utf8]{inputenc}  " << "utf8";
	newRow("multiple") << "\\usepackage[latin1]{inputenc}\n\\usepackage[utf8]{inputenc}" << "latin1";
	newRow("commented") << "  %\\usepackage[latin1]{inputenc}" << "";
	newRow("commented2") << "% \\usepackage[latin1]{inputenc}\n\\usepackage[utf8]{inputenc}" << "utf8";
}

void Test::Encoding::test_getEncodingFromPackage(){

	using namespace Encoding;

	QFETCH(QString,text);
	QFETCH(QString,encodingName);
	QEQUAL(encodingName,Internal::getEncodingFromPackage(text.toLatin1(),text.length(),"inputenc"));
}

void Test::Encoding::test_guessEncoding_data(){

	addColumn<QString>("text");
	addColumn<QString>("encodingName");

	newRow("empty") << "" << "";
	newRow("no info") << "foo bar\nbaz" << "";
	newRow("inputenc") << "\\usepackage[latin1]{inputenc}" << "ISO-8859-1";
	newRow("inputenx") << "\\usepackage[utf8]{inputenx}" << "UTF-8";
}

void Test::Encoding::test_guessEncoding(){

	using namespace Encoding;

	QFETCH(QString, text);
	QFETCH(QString, encodingName);

	QTextCodec * encoding = nullptr;
	int sure = 0;

	guessEncoding(text.toLatin1(),encoding,sure);
	
	if(encodingName.isEmpty()){
        int b = (encoding == nullptr) ? 0 : 1 ;
		QEQUAL(0, b);
	} else {
		QEQUAL(encodingName, QString(encoding->name()));
	}
}


void Test::Encoding::test_encodingEnum(){

	using namespace Encoding;
	
	Q_ASSERT(QTextCodec::codecForName("UTF-8") == QTextCodec::codecForMib(MIB_UTF8));
	Q_ASSERT(QTextCodec::codecForName("UTF-16LE") == QTextCodec::codecForMib(MIB_UTF16LE));
	Q_ASSERT(QTextCodec::codecForName("UTF-16BE") == QTextCodec::codecForMib(MIB_UTF16BE));
	Q_ASSERT(QTextCodec::codecForName("ISO-8859-1") == QTextCodec::codecForMib(MIB_LATIN1));

	Q_ASSERT(QTextCodec::codecForMib(MIB_UTF8) -> mibEnum() == MIB_UTF8);
	Q_ASSERT(QTextCodec::codecForMib(MIB_UTF16LE) -> mibEnum() == MIB_UTF16LE);
	Q_ASSERT(QTextCodec::codecForMib(MIB_UTF16BE) -> mibEnum() == MIB_UTF16BE);
	Q_ASSERT(QTextCodec::codecForMib(MIB_LATIN1) -> mibEnum() == MIB_LATIN1);
}


#endif
