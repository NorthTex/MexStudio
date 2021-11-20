#ifndef Test_Token
#define Test_Token

#ifndef QT_NO_DEBUG


#include "mostQtHeaders.h"
#include "latexparser/latexparser.h"
#include "latexparser/latexreader.h"
#include "testutil.h"
#include <QtTest/QtTest>


const int NW_IGNORED_TOKEN = -2; //token that are not words,  { and }
const int NW_OPTION = -3; //option text like in \include
const int NW_OPTION_PUNCTATION = -4; //option punctation like in \include


class TestToken: public QString {

	static const QRegExp simpleTextRegExp; //defined in testmanager.cpp
	static const QRegExp commandRegExp;
	static const QRegExp ignoredTextRegExp;
	static const QRegExp specialCharTextRegExp;
	static const QRegExp punctationRegExp;

	inline bool matches(const QRegExp & regex){
		return regex.exactMatch(* this);
	}

	inline int matchType(){

		if(matches(simpleTextRegExp))
			return LatexReader::NW_TEXT;

		if(matches(commandRegExp))
			return LatexReader::NW_COMMAND;

		if(matches(punctationRegExp))
			return LatexReader::NW_PUNCTATION;

		if(matches(ignoredTextRegExp))
			return NW_IGNORED_TOKEN;

		if(soll.compare("%"))
			return LatexReader::NW_COMMENT;

		if(matches(specialCharTextRegExp))
			return LatexReader::NW_TEXT;

		return -1;
	}

	void guessType(){

		int match = matchType();

		if(match)
			type = match;
		else
			QVERIFY2(false, QString("invalid test data: \"%1\"").arg(*this).toLatin1().constData());
	}

	public:

		int type , position;

		TestToken()
			: QString()
			, type(NW_IGNORED_TOKEN)
			, position(-1)
			, soll(QString()) {}

		TestToken(const TestToken &token)
			: QString(token)
			, type(token.type)
			, position(token.position)
			, soll(token.soll) {}

		TestToken(const QString &str)
			: QString(str)
			, position(-1)
			, soll(str) { guessType(); }

		TestToken(const char *cstr)
			: QString(cstr)
			, position(-1)
			, soll(QString(cstr)) { guessType(); }

		TestToken(const QString &str, int atype)
			: QString(str)
			, type(atype)
			, position(-1)
			, soll(str) {}

		TestToken(const QString &str,const QString result,int atype)
			: QString(str)
			, type(atype)
			, position(-1)
			, soll(result) {}

		bool operator == (const QString & other){
			return soll.compare(other) == 0;
		}

		bool operator == (const char * other){
			return soll.compare(other) == 0;
		}

		bool operator != (const char * other){
			return soll.compare(other) != 0;
		}

		TestToken & operator = (const TestToken & other) = default;

	private:

		QString soll;
};

Q_DECLARE_METATYPE(TestToken);
Q_DECLARE_METATYPE(QList<TestToken>);


#endif
#endif