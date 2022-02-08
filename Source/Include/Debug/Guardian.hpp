#ifndef Debug_Guardian
#define Debug_Guardian


#include "utilsSystem.h"


class Guardian : public SafeThread {

	Q_OBJECT

	private:

		int slowOperations;

	protected:

		void run();

	public:

		Guardian()
			: SafeThread()
			, slowOperations(0) {}

		static void continueEndlessLoop();
		static void shutdown();
		static void summon();
		static void calm();

		static Guardian * instance();

	public slots:

		void slowOperationStarted();
		void slowOperationEnded();

};

#endif