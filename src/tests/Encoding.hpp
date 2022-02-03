#ifndef Test_Encoding
#define Test_Encoding
#ifndef QT_NO_DEBUG

#include "mostQtHeaders.h"
#include "Include/Encoding.hpp"
#include "tests/Util.hpp"
#include <QtTest/QtTest>
#include "Test.hpp"


testclass(Encoding){

	Q_OBJECT

	testcase( test_lineStart_data );
	testcase( test_lineStart );
	
	testcase( test_lineEnd_data );
	testcase( test_lineEnd );

	testcase( test_getEncodingFromPackage_data );
	testcase( test_getEncodingFromPackage );

	testcase( test_guessEncoding_data );
	testcase( test_guessEncoding );

	testcase( test_encodingEnum );
};


#endif
#endif