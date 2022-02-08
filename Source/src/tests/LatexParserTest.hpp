#ifndef Test_Latex_Parser
#define Test_Latex_Parser

#include "TestToken.hpp"


class LatexParserTest : public QObject {

	Q_OBJECT

	private:

		void nextToken_complex_data();
		void nextToken_complex();
		void nextWordWithCommands_complex_data();
		void nextWordWithCommands_complex();
		void nextWord_complex_data();
		void nextWord_complex();
		void nextTextWord_complex_data();
		void nextTextWord_complex();
		void nextWord_simple_data();
		void nextWord_simple();
		void cutComment_simple_data();
		void cutComment_simple();
		void test_findContext_data();
		void test_findContext();
		void test_findContext2_data();
		void test_findContext2();
		void test_resolveCommandOptions_data();
		void test_resolveCommandOptions();
		void test_findClosingBracket_data();
		void test_findClosingBracket();

	public:

		enum TokenFilter {
			FILTER_NEXTTOKEN,
			FILTER_NEXTWORD_WITH_COMMANDS,
			FILTER_NEXTWORD,
			FILTER_NEXTTEXTWORD
		};

	public:

		TestToken option(const QString & string);
		TestToken env(const QString & string);

	public:

		void addRow(const char * name,TokenFilter,QList<TestToken>);
		void nextWord_complex_test(bool commands);
		void addComplexData(TokenFilter);

};


#endif
