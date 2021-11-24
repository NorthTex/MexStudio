#ifndef Header_DebugHelper
#define Header_DebugHelper

#include "modifiedQObject.h"
#include "smallUsefulFunctions.h"


QString print_backtrace(const QString & message);
QString getLastCrashInformation(bool & wasLoop);

void catchUnhandledException();
void initCrashHandler(int mode);
void recover(); //defined in texstudio.cpp


// class Guardian : public SafeThread {

// 	Q_OBJECT

// 	private:

// 		int slowOperations;

// 	protected:

// 		void run();

// 	public:

// 		Guardian()
// 			: SafeThread()
// 			, slowOperations(0) {}

// 		static void continueEndlessLoop();
// 		static void shutdown();
// 		static void summon();
// 		static void calm();

// 		static Guardian * instance();

// 	public slots:

// 		void slowOperationStarted();
// 		void slowOperationEnded();

// };

#endif