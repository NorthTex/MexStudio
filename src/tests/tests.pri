isEmpty(NO_TESTS){

	message("tests are enabled")

	QT += testlib

	SOURCES +=                                             \
		src/tests/Help.cpp                                 \
		src/tests/CodeSnippet.cpp                          \
		src/tests/Encoding.cpp                             \
		src/tests/ExecProgram.cpp                          \
		src/tests/LatexCompleter.cpp                       \
		src/tests/latexeditorview_bm.cpp                   \
		src/tests/LatexEditorView.cpp                      \
		src/tests/LatexOutputFilter.cpp                    \
		src/tests/LatexParser.cpp                          \
		src/tests/LatexParsing.cpp                         \
		src/tests/QCETestUtil.cpp                          \
		src/tests/DocumentCursor.cpp                       \
		src/tests/DocumentLine.cpp                         \
		src/tests/DocumentSearch.cpp                       \
		src/tests/Editor.cpp                               \
		src/tests/SearchReplacementPanel.cpp               \
		src/tests/ScriptEngine.cpp                         \
		src/tests/Misc.cpp                                 \
		src/tests/StructureView.cpp                        \
		src/tests/SyntaxCheck.cpp                          \
		src/tests/TableManipulation.cpp                    \
		src/tests/UserMacro.cpp                            \
		src/tests/TestManager.cpp                          \
		src/tests/Git.cpp                                  \
		src/tests/Util.cpp


	HEADERS += \
    	src/tests/Help2.hpp \
		src/tests/Test.hpp \
		src/tests/ExecProgram.hpp \
		src/tests/SearchReplacementPanel.hpp \
		src/tests/UpdateChecker.hpp \
		src/tests/DocumentCursor.hpp \
		src/tests/DocumentLine.hpp \
		src/tests/DocumentSearch.hpp \
		src/tests/CodeSnippet.hpp \
		src/tests/LatexCompleter.hpp \
		src/tests/LatexEditorViewBenchmark.hpp \
		src/tests/LatexEditorView.hpp \
		src/tests/LatexOutputFilter.hpp \
		src/tests/LatexParsing.hpp \
		src/tests/LatexStyleParser.hpp \
		src/tests/ScriptEngine.hpp \
		src/tests/Editor.hpp \
		src/tests/BuildManager.hpp \
		src/tests/TableManipulation.hpp \
		src/tests/Misc.hpp \
		src/tests/UtilUI.hpp \
		src/tests/UtilVersion.hpp \
		src/tests/Encoding.hpp \
		src/tests/SyntaxChecker.hpp \
		src/tests/QCETestUtil.hpp \
		src/tests/TestManager.hpp \
		src/tests/Util.hpp \
		src/tests/UserMacro.hpp \
		src/tests/Git.hpp \
		src/tests/StructureView.hpp

} else {

	message("tests are disabled")

	DEFINES += NO_TESTS

}

HEADERS += \
    $$PWD/LatexParserTest.hpp \
    $$PWD/TestToken.hpp

