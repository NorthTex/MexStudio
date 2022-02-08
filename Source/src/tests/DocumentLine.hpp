#ifndef Test_DocumentLine
#define Test_DocumentLine

#ifndef QT_NO_DEBUG

#include "mostQtHeaders.h"
#include "Test.hpp"

class QDocument;
class QDocumentLine;
class QDocumentLineHandle;

testclass(DocumentLine){

	Q_OBJECT

	private:

		QDocument * doc;

		bool savedFixedPitch;
		
		int
			savedSpaceWidth,
			savedLeftPadding;
		
		bool
			savedWorkAroundFPM,
			savedWorkAroundFSCD,
			savedWorkAroundFQTL;

	private slots:

		void initTestCase();
		void cleanupTestCase();

		testcase( updateWrap_data );
		testcase( updateWrap );

	public:

		DocumentLine();
		~DocumentLine();

};


#endif
#endif
