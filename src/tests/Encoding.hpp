#ifndef Test_Encoding
#define Test_Encoding
#ifndef QT_NO_DEBUG

#include "mostQtHeaders.h"
#include "encoding.h"
#include "tests/Util.hpp"
#include <QtTest/QtTest>

class EncodingTest : public QObject
{
	Q_OBJECT
private slots:
	void test_lineStart_data();
	void test_lineStart();
	void test_lineEnd_data();
	void test_lineEnd();

	void test_getEncodingFromPackage_data();
	void test_getEncodingFromPackage();
	void test_guessEncoding_data();
	void test_guessEncoding();

	void test_encodingEnum();
};

#endif // QT_NO_DEBUG
#endif