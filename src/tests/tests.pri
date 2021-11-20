isEmpty(NO_TESTS){

	message("tests are enabled")

	QT += testlib

	SOURCES += \
		src/tests/Help.cpp \
		src/tests/CodeSnippet.cpp \
		src/tests/Encoding.cpp \
		src/tests/ExecProgram.cpp \
		src/tests/LatexCompleter.cpp \
		src/tests/latexeditorview_bm.cpp \
		src/tests/LatexEditorView.cpp \
		src/tests/LatexOutputFilter.cpp \
		src/tests/LatexParser.cpp \
		src/tests/LatexParsing.cpp \
		src/tests/QCETestUtil.cpp \
		src/tests/DocumentCursor.cpp \
		src/tests/DocumentLine.cpp \
		src/tests/DocumentSearch.cpp \
		src/tests/Editor.cpp \
		src/tests/SearchReplacementPanel.cpp \
		src/tests/ScriptEngine.cpp \
		src/tests/Misc.cpp \
		src/tests/StructureView.cpp \
		src/tests/SyntaxCheck.cpp \
		src/tests/TableManipulation.cpp \
		src/tests/UserMacro.cpp \
		src/tests/TestManager.cpp \
		src/tests/Git.cpp \
		src/tests/Util.cpp


	HEADERS += \
                src/tests/Test.hpp \
		src/tests/execprogram_t.h \
		src/tests/qsearchreplacepanel_t.h \
		src/tests/updatechecker_t.h \
		src/tests/qdocumentcursor_t.h \
		src/tests/qdocumentline_t.h \
		src/tests/qdocumentsearch_t.h \
		src/tests/codesnippet_t.h \
		src/tests/latexcompleter_t.h \
		src/tests/latexeditorview_bm.h \
		src/tests/latexeditorview_t.h \
		src/tests/latexoutputfilter_t.h \
		src/tests/latexparsing_t.h \
		src/tests/latexstyleparser_t.h \
		src/tests/scriptengine_t.h \
		src/tests/qeditor_t.h \
		src/tests/buildmanager_t.h \
		src/tests/tablemanipulation_t.h \
		src/tests/smallUsefulFunctions_t.h \
		src/tests/utilsui_t.h \
		src/tests/utilsversion_t.h \
		src/tests/encoding_t.h \
		src/tests/help_t.h \
		src/tests/syntaxcheck_t.h \
		src/tests/qcetestutil.h \
		src/tests/testmanager.h \
		src/tests/testutil.h \
		src/tests/usermacro_t.h \
                src/tests/git_t.h \
                src/tests/structureview_t.h

} else {

	message("tests are disabled")

	DEFINES += NO_TESTS

}

HEADERS += \
    $$PWD/LatexParserTest.hpp \
    $$PWD/TestToken.hpp

