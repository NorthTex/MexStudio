
#include "App.hpp"
#include "CommandLine.hpp"
#include <iostream>


/*!
 *	@brief Print C++ Version
 */

inline void printCppVersion(){

	using std::cout;
	using std::endl;

	cout 
		<< "Using C++ " 
		<< __cplusplus 
		<< endl;
}


/*!
 *	@brief Entrypoint to MexStudio
 */

int main(int count,char ** arguments){

	printCppVersion();


	auto app = App(count,arguments);
	
	auto [ commandLine , allowSeparateInstance ] = parseArguments();

	const bool alreadyDone = handleCommandLine(commandLine);

	if(alreadyDone)
		return 0;

	if(!allowSeparateInstance && app.isRunning()){
		app.instructOthers(commandLine);
		return 0;
	}

	app.setApplicationName("MexStudio");
	app.setDesktopFileName("mexstudio");
	app.startup(commandLine);

	return app.run();
}
