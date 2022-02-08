
#include "configmanager.h"
#include "Debug/Logger.hpp"


/*!
 *  @brief Extract info / Format arguments
 */

std::tuple<QStringList,bool> parseArguments(){

	bool allowSeparateInstance = false;
	QStringList commandLine;


	const auto arguments = QCoreApplication::arguments();
	const auto count = arguments.count();

	for(int i = 1;i < count;++i){

		const auto argument = arguments[i];
		const auto hasNext = i + 1 < count;


		if(argument.startsWith('-')){

			if(argument == "--start-always"){
				allowSeparateInstance = true;
                continue;
            }

			if(argument == "--no-session"){
				ConfigManager::dontRestoreSession = true;
                continue;
            }

            if(hasNext){

                const auto value = arguments[++i];

                if(argument == "-line" || argument == "--line"){
                    commandLine << "--line" << value;
                    continue;
                }

                if(argument == "-page" || argument == "--page"){
                    commandLine << "--page" << value;
                    continue;
                }

                if(argument == "-insert-cite" || argument == "--insert-cite"){
                    commandLine << "--insert-cite" << value;
                    continue;
                }

                if(argument == "--config"){
                    ConfigManager::configDirOverride = value;
                    continue;
                }

                if(argument == "--debug-logfile"){

                    #ifdef DEBUG_LOGGER
                        DebugLogger::Start(value);
                    #endif

                    continue;
                }

            } else {
                commandLine << argument;
                continue;
            }
        }


        if(argument.endsWith(".txss"))
           ConfigManager::dontRestoreSession = true;

        commandLine
            << QFileInfo(argument).absoluteFilePath();
	}

	return { commandLine , allowSeparateInstance };
}


const char
    * magenta = "\033[35m",
    * yellow = "\033[33m",
    * reset = "\033[0m",
    * green = "\033[32m",
    * blue = "\033[34m",
    * cyan = "\033[36m",
    * gray = "\033[90m",
    * red = "\033[31m";


/*!
 *  @brief Format argument with parameter
 */

inline QString formatArgument(
    const char * argument,
    const char * value,
    const char * description
){
    return std::string(
        "\n  " + std::string(yellow) + "--" + argument + std::string(green) + " " + value +
        "\n  " + std::string(gray) + description + "\n"
    ).c_str();
}


/*!
 *  @brief Format argument without parameter
 */

inline QString formatArgument(
     const char * argument,
     const char * description
){
    return formatArgument(argument,"",description);
}


/*!
 *  @brief Print Help Dialog
 */

inline void printHelp(){

    QTextStream(stdout)
        << "\n"
        << red << "Usage: \n" << reset
        << "\n"
        << magenta << "  mexstudio " << blue << "[Options] [File]\n"
        << "\n"
        << red << "Options:\n" << reset
        << formatArgument( "version" , "Print MexStudios version." )
        << formatArgument( "pdf-viewer-only" , "Only use the PDF Viewer." )
        << formatArgument( "start-always" , "Start a separate instance of MexStudio." )
        << formatArgument( "no-session" , "Use a private session that won't retain any data." )
        << formatArgument( "master" , "Mark your document as root document." )
        << formatArgument(
            "config","<Path>",
            "Specify the path to the settings directory." )
        << formatArgument(
            "line","<Line>:<Column>",
            "Place the cursor at the given line / and column." )
        << formatArgument(
            "insert-cite","<Citation>",
            "Inserts the specified citation." )
        << formatArgument(
            "page","<Page>",
            "Show the specified page in the PDF Viewer." )
        << formatArgument(
            "texpath","<Path>",
            "Uses the given path as first search directory when force resetting." );
}


/*!
 *  @brief Print MexStudios Version
 */

inline void printVersion(){
    QTextStream(stdout)
        << "TeXstudio "
        << TXSVERSION
        << " ("
        << TEXSTUDIO_GIT_REVISION
        << ")\n";
}


/*!
 *  @brief Handle commands that don't run MexStudio
 *  @return If the MexStudio doesn't need to be started
 *  @note Windows GUI does not support stdout
 */

inline bool handleCommandLine(const QStringList & commandLine){
    
    if(commandLine.contains("--help")){
        printHelp();
        return true;
    }

    if(commandLine.contains("--version")){
        printVersion();
        return true;
    }

    return false;
}
