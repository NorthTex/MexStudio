#include "smallUsefulFunctions.h"
#include "qdocumentline_p.h"
#include "qdocument.h"
#include <QBuffer>
#include "latexparser/latexparser.h"
#include "TransformCharacters.hpp"


/*!
 * \brief transformCharacter
 * Transform a character from a tex encoded to utf
 * e.g. "a -> ä
 * \param c
 * \param context
 * \return tranformed character
 */

QChar transformCharacter(
	const QChar & character,
	const QChar & context
){
    const auto transformation = characters.find({
		context.toLatin1() , character.toLatin1() 
	});

    if(transformation == characters.end())
        return character;

    return QChar(transformation -> second);
}

QString latexToPlainWord(const QString & word){

	QString result;

	result.reserve(word.length());

	for(int i = 0;i < word.length();i++){
		if(word[i] == '\\'){

			// decode all meta characters starting with a backslash
			// (c++ syntax: don't use an actual backslash there or it creates a multi line comment)

			i++;

			if(i >= word.length())
				break;

			switch(word[i].toLatin1()){
			case '-': //Trennung [separation] (german-babel-package also: \")
			case '/': //ligatur preventing (german-package also: "|)
				break;

			case '"':
			case '\'':
			case '^':
			case '`':
			case '~':
			case 'c':

				if(i + 3 < word.length())
					if(word[i + 1] == '{' && word[i + 3] == '}'){
						result.append(transformCharacter(word[i + 2],word[i]));
						i += 3;

						break;
					}

				if(i + 1 < word.length()){
					
					if(word[i + 1] == '\\')
						break;
					
					if(word[i + 1] == '"')
						break;  //ignore "

					result.append(transformCharacter(word[i + 1],word[i]));
					i++;

					break;
				}

				i--; //repeat with "

				break;
			default:
				i--; //repeat with current char
			}
		} else {
			result.append(word[i]);
		}
	}

	return result;
}


QString latexToPlainWordwithReplacementList(
	const QString & latex,
	QMap<QString,QString> & replacementList
){
	QString result;
	QString word = latexToPlainWord(latex);

    if(replacementList.isEmpty()){
        result = word;
	} else {
        while(!word.isEmpty()){

            bool replaced = false;

            for(const auto & replacement : replacementList.keys())
                if(word.startsWith(replacement)){
                    result.append(replacementList.value(replacement));
                    word = word.mid(replacement.length());
                    replaced = true;
                    break;
                }

            if(!replaced){
                result.append(word.left(1));
                word = word.mid(1);
            }
        }
    }

    // remove leading and trailing "

    if(result.startsWith("\""))
        result = result.mid(1);

    if(result.endsWith("\""))
        result.chop(1);

    return result;
}


using StringPair = QPair<QString,QString>;


QString textToLatex(const QString & text){

	QVector<StringPair> replaceList;

	// replacements for resevered characters according to
	// http://en.wikibooks.org/wiki/LaTeX/Basics#Reserved_Characters

	QString result = text;

    result.replace("{","\\{");
    result.replace("}","\\}");
    result.replace(QRegularExpression("\\\\(?![{}])"),"\\textbackslash{}");

	replaceList.append(StringPair("#","\\#"));
	replaceList.append(StringPair("$","\\$"));
	replaceList.append(StringPair("%","\\%"));
	replaceList.append(StringPair("&","\\&"));
	replaceList.append(StringPair("~","\\~{}"));
	replaceList.append(StringPair("_","\\_"));
	replaceList.append(StringPair("^","\\^{}"));


	for(auto && replace : qAsConst(replaceList))
		result.replace(replace.first,replace.second);

	result.replace(QRegularExpression("\"(.*?)\""),"``\\1''");

	return result;
}


int startOfArg(const QString & argument,int index){

	for(int i = index; i < argument.length(); i++){

		auto & c = argument.at(i);

		if(c.isSpace())
			continue;

		if(c == '{')
			return i;
			
		break;
	}

	return -1;
}


/*!
 * Parses a Latex string to a plain string.
 * Specifically, this substitues \texorpdfstring and removes explicit hyphens.
 */

QString latexToText(QString latex){

	// substitute \texorpdfstring

	int start , stop;
	int texorpdfstringLength = 15;

	start = latex.indexOf("\\texorpdfstring");

	while(start >= 0 && start < latex.length()){

		// first arg

		int i = startOfArg(latex,start + texorpdfstringLength);

		// no arguments for \\texorpdfstring

		if(i < 0){
			start += texorpdfstringLength;
			start = latex.indexOf("\\texorpdfstring",start);
			continue;
		}

		i++;

		stop = findClosingBracket(latex,i);

		// missing closing bracket for first argument of \\texorpdfstring

		if(stop < 0){
			start += texorpdfstringLength;
			start = latex.indexOf("\\texorpdfstring",start);
			continue;
		}

		// second arg

		i = startOfArg(latex,stop + 1);

		// no second arg for \\texorpdfstring

		if(i < 0){
			start += texorpdfstringLength;
			start = latex.indexOf("\\texorpdfstring",start);
			continue;
		}

		i++;

		stop = findClosingBracket(latex,i);

		// no second arg for \\texorpdfstring

		if(stop < 0){
			start += texorpdfstringLength;
			start = latex.indexOf("\\texorpdfstring",start);
			continue;
		}

		latex.remove(stop, 1);
		latex.remove(start,i - start);
		start = latex.indexOf("\\texorpdfstring",start);
	}

	// remove discretionary  hyphenations

	latex.remove("\\-");
	return latex;
}


// joins all the input lines trimming whitespace.
// A new line is started on comments and empty lines

QStringList joinLinesExceptCommentsAndEmptyLines(const QStringList & lines){

	QStringList joinedLines;
	QString tmpLine;

    const auto flush = [ & ](){
        if(!tmpLine.isEmpty()){
            joinedLines.append(tmpLine);
            tmpLine.clear();
        }
    };

	for(const auto & line : lines){

		QString rtrimmedLine = trimRight(line);

		// empty line as separator

		if(rtrimmedLine.isEmpty()){
			flush();
			joinedLines.append(rtrimmedLine);
			continue;
		}

		if(tmpLine.isEmpty())
			tmpLine.append(rtrimmedLine);
		else
			tmpLine.append(" " + rtrimmedLine.trimmed());

		int commentStart = LatexParser::commentStart(rtrimmedLine);

        if(commentStart >= 0)
            flush();
	}

	flush();

	return joinedLines;
}


// splits lines after maximal number of chars 
// while keeping track of indentation and comments

QStringList splitLines(
	const QStringList & lines,
	int maxCharPerLine,
	const QRegularExpression & breakChars
){

	QStringList splittedLines;

	int maxIndent = maxCharPerLine / 2 * 3;

	for(auto line : lines){

		int textStart = 0;

		while(
			textStart < line.length() && 
			line.at(textStart).isSpace() && 
			textStart < maxIndent
		)	textStart++;

		// empty line

		if(textStart >= line.length()){
			splittedLines << line;
			continue;
		}

		int maxCharPerLineWithoutIndent = maxCharPerLine - textStart;

		QString indent = line.left(textStart);
		line = line.mid(textStart);

		bool inComment = false;

		while(line.length() > maxCharPerLineWithoutIndent){

			if(inComment)
				line.prepend("% ");

			int breakAt = line.lastIndexOf(breakChars, maxCharPerLineWithoutIndent);

			if(breakAt <= 3)
				breakAt = -1;

			QString leftPart = line.left(breakAt);
			splittedLines << indent + leftPart;

			if(breakAt < 0){
				line.clear();
				break;
			}

			line.remove(0, breakAt + 1);

			inComment = inComment || (LatexParser::commentStart(leftPart) >= 0);
		}

		if(line.length() > 0){
			
			if(inComment)
				line.prepend("% ");
			
			splittedLines << indent + line;
		}
	}

	return splittedLines;
}


bool localeAwareLessThan(const QString & a,const QString & b){
	return QString::localeAwareCompare(a,b) < 0;
}


/*!
 *  \brief Trims whitespace from the start of the given string.
 */

QString trimLeft(const QString & string){

	for(int i = 0; i < string.length();i++)
		if(!string[i].isSpace())
			return string.mid(i);

	return string;
}


/*!
 *  \brief Trims whitespace from the end of the given string.
 */

QString trimRight(const QString & string){

	for(int i = string.length();i > 0;i--)
		if(!string[i - 1].isSpace())
			return string.left(i);

	return string;
}


/*!
 * \brief get argument after command 'token'
 *
 * handles latex comments correctly
 * \warning obsolete with lexer based token system
 * \param line text of one line
 * \param token latexcommand
 * \return text after token
 */

QString findToken(
	const QString & line,
	const QString & token
){
	int tagStart = line.indexOf(token);

	// find start of comment (if any)

    int commentStart = line.indexOf(QRegularExpression("(^|[^\\\\])%"));

    if(tagStart != -1 && (commentStart > tagStart || commentStart == -1)){

		tagStart += token.length();

		int tagEnd = line.indexOf("}", tagStart);

		if(tagEnd != -1)
			tagEnd -= tagStart;

		return line.mid(tagStart,tagEnd);
	}

	return "";
}


/*!
 * \brief get argument after command 'token'
 *
 * handles latex comments correctly
 * \warning obsolete with lexer based token system
 * \param line text of one line
 * \param token latexcommand
 * \param start column number
 * \return text after token
 */

QString findToken(
	const QString & line,
	const QString & token,
	int & start
){
	int tagStart = line.indexOf(token,start);

	// find start of comment (if any)

    int commentStart = line.indexOf(QRegularExpression("(^|[^\\\\])%")); 

	if(tagStart != -1 && (commentStart > tagStart || commentStart == -1)){
		tagStart += token.length();
		int tagEnd = line.indexOf("}",tagStart);
		start = tagStart;
		
		// return everything after line if there is no }

		if(tagEnd == -1)
			return line.mid(tagStart);

		return line.mid(tagStart,tagEnd - tagStart);
	}

	start = -2;
	return "";
}


/*!
 * \brief get argument after command 'token'
 *
 * handles latex comments correctly
 * \warning obsolete with lexer based token system
 * \param line text of one line
 * \param token regexp to search
 * \return text after token
 */

QString findToken(
	const QString & line,
	const QRegExp & token
){

	int tagStart = 0;
	QString string = line;
	tagStart = token.indexIn(line);

	// find start of comment (if any)

    int commentStart = line.indexOf(QRegularExpression("(^|[^\\\\])%"));

	if(tagStart == -1)
		return "";

	if(commentStart > tagStart || commentStart == -1){
		string = string.mid(tagStart + token.cap(0).length(),string.length());
		return string;
	}

	return "";
}


bool findTokenWithArg(
	const QString & line,
	const QString & token,
	QString & outName,
	QString & outArg
){
	outName = "";
	outArg = "";

    int tagStart = line.indexOf(token);

	// find start of comment (if any)

    int commentStart = line.indexOf(QRegularExpression("(^|[^\\\\])%"));

	if(tagStart == -1)	
		return false;

    if(commentStart > tagStart || commentStart == -1){

		tagStart += token.length();

		int tagEnd = line.indexOf("}",tagStart);

		if(tagEnd == -1){
			//return everything after line if there is no }
			outName = line.mid(tagStart);
		} else {

			outName = line.mid(tagStart,tagEnd - tagStart);

			int curlyOpen = line.indexOf("{",tagEnd);
			int optionStart = line.indexOf("[",tagEnd);

			if(optionStart < curlyOpen || (curlyOpen == -1 && optionStart != -1)){

				int optionEnd = line.indexOf("]",optionStart);

				outArg = (optionEnd == -1)
					? line.mid(optionStart + 1)
					: line.mid(optionStart + 1,optionEnd - optionStart - 1);
			}
		}

		return true;
	}

	return false;
}


/*! returns the command at pos (including \) in outCmd. pos may be anywhere in the command name (including \) but
 * not in command options. Return value is the index of the first char after the command (or pos if there was no command
 * \warning obsolete with lexer-based token system
 */

// TODO: currently does not work for command '\\'

int getCommand(
	const QString & line,
	QString & outCmd,
	int pos
){

	int start = pos;

	// find beginning

	while(line.at(start) != '\\'){

		if(!isCommandChar(line.at(start)))
			return pos;
			
		if(start == 0)
			return pos;

		start--;
	}

	int i = pos + 1;

	for(;i < line.length();i++)
		if(!isCommandChar(line.at(i)))
			break;

	outCmd = line.mid(start,i - start);

	return i;
}


/*! returns command option list. pos has to be at the beginning of the first bracket
 * posBehind returns the position after the last bracket, you may pass the same variable as in pos
 * \warning obsolete with lexer-based token system
 */

using CharMap = QMap<QChar,QChar>;

QList<CommandArgument> getCommandOptions(
	const QString & line,
	int pos,
	int * posBehind
){

	static CharMap cbs;

	if(cbs.isEmpty()){
		cbs[QChar('{')] = QChar('}');
		cbs[QChar('[')] = QChar(']');
	}

	QList<CommandArgument> options;

	int start = pos;

	if(posBehind)
		* posBehind = start;

	if(pos >= line.length())
		return options;

	QChar oc = line[start];

	if(!cbs.contains(oc))
		return options;

	for(int num = 1;;num++){

		int end = findClosingBracket(line,start,oc,cbs[oc]);

		// open without close

		if(end < 0)
			break;

		CommandArgument arg;

		arg.isOptional = (oc == '[');
		arg.number = num;
		arg.value = line.mid(start + 1,end - start - 1);
		options.append(arg);
		start = end + 1;

		if(posBehind)
			* posBehind = start;

		// close on last char or last option reached

		if(start >= line.length())
			break;
			
		if(!cbs.contains(line[start]))
			break;

		oc = line[start];
	}

	return options;
}


/* returns the item at pos in a colon separated list of options (empty on colon
 * e.g. getParamItem("{one, two, three}", 7) returns "two"
 * \warning obsolete with lexer-based token system
 */

QString getParamItem(const QString & line,int pos,bool stopAtWhiteSpace){

	REQUIRE_RET(pos <= line.length(),QString());

	int start;
	int curlCount = 0;
	int squareCount = 0;

	QString openDelim(",{[");

	if(stopAtWhiteSpace)
		openDelim += " \t\n\r";

	for(start = pos;start > 0;start--){

		QChar c = line.at(start - 1);

		if(c == '}' && openDelim.contains('{'))
			curlCount++;

		if(c == '{'){

			if(curlCount-- > 0)
				continue;

			break;
		}

		if(c == ']' && openDelim.contains('['))
			squareCount++;

		if(c == '['){

			if(squareCount-- > 0)
				continue;

			break;
		}

		if(openDelim.contains(c))
			break;
	}

	int end = pos;
	QString closeDelim(",]}");

	if(stopAtWhiteSpace)
		closeDelim += " \t\n\r";

	curlCount = 0;
	squareCount = 0;

	for(end = pos; end < line.length(); end++){

		QChar c = line.at(end);

		if(c == '{' && closeDelim.contains('}'))
			curlCount++;

		if(c == '}'){

			if(curlCount-- > 0)
				continue;

			break;
		}

		if(c == '[' && closeDelim.contains(']'))
			squareCount++;

		if(c == ']'){

			if(squareCount-- > 0)
				continue;

			break;
		}

		if(closeDelim.contains(c))
			break;
	}

	return line.mid(start,end - start);
}


QRegExp generateRegExp(
	const QString & text,
	const bool isCase,
	const bool isWord,
	const bool isRegExp
){

	Qt::CaseSensitivity cs = isCase 
		? Qt::CaseSensitive 
		: Qt::CaseInsensitive;
	
	QRegExp m_regexp;

	if(isRegExp)
		return QRegExp(text,cs,QRegExp::RegExp);

	//todo: screw this? it prevents searching of "world!" and similar things
	//(qtextdocument just checks the surrounding character when searching for whole words, this would also allow wholewords|regexp search)

	if(isWord)
		return QRegExp(QString("\\b%1\\b").arg(QRegExp::escape(text)),cs,QRegExp::RegExp);

	return QRegExp(text,cs,QRegExp::FixedString);
}


QStringList regExpFindAllMatches(
	const QString & string,
	const QRegExp & regex,
	int cap
){
	int offset = regex.indexIn(string);

	QStringList matches;

	while(offset > -1){
		matches << regex.cap(cap);
		offset = regex.indexIn(string,offset + regex.matchedLength());
	}

	return matches;
}


/*!
 * a multi-match equivalent of QString::indexOf(QString)
 */

QList<int> indicesOf(
	const QString & line,
	const QString & word,
	Qt::CaseSensitivity useCase
){
	QList<int> columns;
	int column = 0;

	while(column < line.length() - 1){

		column = line.indexOf(word,column,useCase);
		
		if(column < 0)
			break;
		
		columns.append(column);
		column++;
	}

	return columns;
}


/*!
 * a multi-match equivalent of QString::indexOf(QRegExp)
 */

QList<int> indicesOf(
	const QString & line,
	const QRegularExpression & regex
){
	QList<int> columns;
	int column = 0;

	// exact match

	while(column < line.length() - 1){
	
		column = line.indexOf(regex,column);
	
		if(column < 0)	
			break;
	
		columns.append(column);
		column++;
	}

	return columns;
}


/*! adds entries for structure commands to the Dom of a QNFA file
 * commands are taken from possibleCommands["%structure0"] to possibleCommands["%structureN"]
 */

void addStructureCommandsToDom(
	QDomDocument & document,
	const QHash<QString,QSet<QString>> & possibleCommands
){
	QDomElement root = document.documentElement();
	auto nodes = root.childNodes();

	QDomNode parent;

	for(int i = nodes.size() - 1;i >= 0;i--){
	
		const auto node = nodes.item(i);

		const auto id = node
			.attributes()
			.namedItem("id")
			.nodeValue();
	
		if(id == "keywords/structure"){
			parent = node;
			break;
		}
	}

	if(parent.isNull())
		return;

	while(!parent.firstChild().isNull())
		parent.removeChild(parent.firstChild());

	for(int level = 0;level <= LatexParser::MAX_STRUCTURE_LEVEL;level++)
		for(const auto & cmd : possibleCommands[QString("%structure%1").arg(level)]){

			QDomElement child = document.createElement("word");
			
			QString name = cmd;
			name.remove('\\');
			
			child.setAttribute("parenthesis", QString("structure%1:boundary@nomatch").arg(level));
			child.setAttribute("parenthesisWeight", QString("%1").arg(8 - level));
			child.setAttribute("fold", "true");
			
			name = cmd;
			name.replace('\\', "\\\\");  // words are regexps, so we have to escape the slash
			
			QDomText dtxt = document.createTextNode(name);

			child.appendChild(dtxt);
			parent.appendChild(child);
		}
}



/*!
 * \brief convert a list of integer in one string with a textual representation of said integers
 *
 * The numbers are given as text, separated by commas
 * \param ints list of integer
 * \return string containg a textual list of integers
 */

QString intListToStr(const QList<int> & numbers){

	QString string = "";
	
	for(int number : numbers)
		string.append(QString::number(number) + ',');

	// remove last ','

	if(string.length() > 0)
		string.remove(string.length() - 1,1);
	
	return string;
}


QList<int> strToIntList(const QString & string){

	QList<int> list;
	
	for(const auto & part : string.split(',')){

		bool ok;
		
		int number = part.toInt(& ok);
		
		if(ok)
			list << number;
	}

	return list;
}


QString enquoteStr(const QString & string){

	QString enquoted = string;
	enquoted.replace('"', "\\\"");
	enquoted.prepend('"');
	enquoted.append('"');
	
	return enquoted;
}


QString dequoteStr(const QString & string){
	
	QString dequoted = string;
	
	if(dequoted.endsWith('"') && !dequoted.endsWith("\\\""))
		dequoted.remove(dequoted.length() - 1,1);
	
	if(dequoted.startsWith('"'))
		dequoted.remove(0,1);
	
	dequoted.replace("\\\"","\"");
	
	return dequoted;
}


/** add a quotation around the string if it does not already have one. **/

QString quotePath(const QString & string){
	
	if(string.startsWith('"'))
		return QString(string);
		
	if(!string.contains(' '))
		return QString(string);
	
	return QString("\"%1\"")
		.arg(string);
}


/** if the string is surrounded by qoutes, remove these **/

QString removeQuote(const QString & string){

	const auto length = string.length();

	if(length < 2)
		return string;

	if(!string.startsWith('\''))
		return string;

	if(!string.startsWith('\''))
		return string;

	return string.mid(1,length - 2);
}


// we use the explicit chars intentionally and not QDir::separator()
// because it shall also work for / on windows (many paths are internally
// represented with / as delimiter

QString removePathDelim(const QString & path){
	
	if(path.endsWith('/'))
		return path.left(path.length() - 1);
	
	if(path.endsWith('\\'))
		return path.left(path.length() - 1);

	return path;
}


QString removeAccents(const QString & string){

	static const auto diacriticLetters = QString::fromUtf8("ŠŒŽšœžŸ¥µÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ");
	static const auto noDiacriticLetters = QStringList() 
		<< "S" << "OE" << "Z" << "s" << "oe" << "z" << "Y" << "Y" 
		<< "u" << "A" << "A" << "A" << "A" << "A" << "A" << "AE"
		<< "C" << "E" << "E" << "E" << "E" << "I" << "I" << "I"
		<< "I" << "D" << "N" << "O" << "O" << "O" << "O" << "O"
		<< "O" << "U" << "U" << "U" << "U" << "Y" << "s" << "a"
		<< "a" << "a" << "a" << "a" << "a" << "ae" << "c" << "e"
		<< "e" << "e" << "e" << "i" << "i" << "i" << "i" << "o"
		<< "n" << "o" << "o" << "o" << "o" << "o" << "o" << "u" 
		<< "u" << "u" << "u" << "y" << "y";

	QString output = "";

	for(QString c : string){

		const auto index = diacriticLetters.indexOf(c);

		if(index > -1)
			c = noDiacriticLetters.at(index);

		output.append(c);
	}

	return output;
}


QString makeLatexLabel(const QString & string){

	auto normalized = removeAccents(string)
		.normalized(QString::NormalizationForm_KD)
		.toLower();
	
	normalized.replace(' ', '-');
    normalized.remove(QRegularExpression("[^a-z0-9\\-]"));
	
	return normalized;
}


/*! Splits a command string into the command an arguments.
 *  This respects quoted arguments. Output redirection operators are separate tokens
 */
QStringList tokenizeCommandLine(const QString & commandLine){

    QStringList result;

	QString currentToken = "";
	currentToken.reserve(30);
	
	bool 
		inQuote = false,
		escape = false;

	const auto flush = [ & ](auto & value){
		
		if(!value.isEmpty())
            result << value;

        value = "";
	};

	for(const auto & c : commandLine){

		if(c.isSpace()){
			if(inQuote)
				currentToken.append(c);
			else {
                flush(currentToken);
			}
		} else
		if(c == '\\'){
			escape = ! escape;
			currentToken.append(c);
			continue;
		} else
		if(c == '"'){
			
			if(!escape)
				inQuote = ! inQuote;
			
			currentToken.append(c);
		} else
		if(c == '>'){
			if(inQuote){
				currentToken.append(c);
			} else
			if(currentToken == "2"){
				currentToken.append(c);
                flush(currentToken);
			} else {
                flush(currentToken);
				currentToken = c;
                flush(currentToken);
			}
		} else {
			currentToken.append(c);
		}

		escape = false;
	}

    flush(currentToken);

	return result;
}


QStringList extractOutputRedirection(const QStringList & arguments,QString & output,QString & error){
	
	QStringList redirections;
	bool finished = false;
	
	for(int i = 0;i < arguments.length();i++){

		const auto argument = arguments[i];

		if(argument == ">" && i < arguments.length() - 1){
			output = arguments[i + 1];
			i++;
			finished = true;
			continue;
		}
		
		if(argument.startsWith(">")){
			output = argument.mid(1);
			finished = true;
			continue;
		}
		
		if(argument == "2>" && i < arguments.length() - 1){
			error = arguments[i + 1];
			i++;
			finished = true;
			continue;
		}

		if(argument.startsWith("2>")){
			error = argument.mid(2);
			finished = true;
			continue;
		}

		if(!finished)
			redirections << argument;
	}

	return redirections;
}


uint joinUnicodeSurrogate(const QChar & high,const QChar & low){

	return 0x10000
		+ ((high.unicode() & 0x03FF) << 10)
		+  ( low.unicode() & 0x03FF);
}


QString getImageAsText(const QPixmap & image,const int width){
	
	QByteArray bytes;
	
	QBuffer buffer(& bytes);
	buffer.open(QIODevice::WriteOnly);

	image.save(& buffer,"PNG");

	const auto data = QString(buffer.data().toBase64());

	QString length = "";

	if(width >= 0)
		length = QString(" width=%2 ")
			.arg(width);

	return QString("<img src=\"data:image/png;base64,%1\"%2>")
		.arg(data)
		.arg(length);
}


/*!
 * Shows a tooltip at the given position (pos = top left corner).
 * If the tooltip does not fit on the screen, it's attempted to position it to the left including
 * a possible relatedWidgetWidth offset (pos - relatedWidgetWidth = top right corner).
 * If there is not enough space as well the text is shown in the position (left/right) of maxium
 * available space and the text lines are shortend to fit the available space.
 */

void showTooltipLimited(QPoint pos, QString text, int relatedWidgetWidth){

	//if there are tabs at the position in the string, qt crashes. (13707)

	text.replace("\t", "    ");

	QRect screen = UtilsUi::getAvailableGeometryAt(pos);

	// estimate width of coming tooltip
	// rather dirty code

	bool textWillWarp = Qt::mightBeRichText(text);

    QLabel lLabel(nullptr, Qt::ToolTip);
	lLabel.setFont(QToolTip::font());
	lLabel.setMargin(1 + lLabel.style() -> pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, &lLabel));
	lLabel.setFrameStyle(QFrame::StyledPanel);
	lLabel.setAlignment(Qt::AlignLeft);
	lLabel.setIndent(1);
	lLabel.setWordWrap(textWillWarp);
	lLabel.ensurePolished();
	lLabel.setText(text);
	lLabel.adjustSize();

	int textWidthInPixels = lLabel.width() + 10; // +10 good guess

	if (pos.x() - screen.x() + textWidthInPixels <= screen.width()) {
		// tooltip fits at the requested position
		QToolTip::showText(pos, text);
	} else {

		// try positioning the tooltip left of the releated widget

		QPoint posLeft(pos.x() - textWidthInPixels - relatedWidgetWidth, pos.y());

		if(posLeft.x() >= screen.x()){
			QToolTip::showText(posLeft,text);
		} else {

			// text does not fit to the left
			// choose the position left/right with the maximum available space

			int availableWidthLeft = (pos.x() - screen.x()) - relatedWidgetWidth;
			int availableWidthRight = screen.width() - (pos.x() - screen.x());
			int availableWidth = qMax(availableWidthLeft, availableWidthRight);

			bool positionLeft = availableWidthLeft > availableWidthRight;

			if(!textWillWarp){

				// shorten text lines to fit textwidth (only feasible if the tooltip does not wrap)

				QStringList lines = text.split("\n");
				int maxLength = 0;
				QString maxLine;

				for(const auto & line : lines)
					if(line.length() > maxLength){
						maxLength = line.length();
						maxLine = line;
					}

				int averageWidth = lLabel.fontMetrics().averageCharWidth();

                if(averageWidth > 1)
                    maxLength = qMin(maxLength, availableWidth / averageWidth);

                while(textWidthInPixels > availableWidth && maxLength > 10){

                    maxLength -= 2;

					for(int i = 0; i < lines.count(); i++)
						lines[i] = lines[i].left(maxLength);

					lLabel.setText(lines.join("\n"));
					lLabel.adjustSize();
					textWidthInPixels = lLabel.width() + 10;
				}

				text = lines.join("\n");
			}

			if(positionLeft){
				posLeft.setX(pos.x() - textWidthInPixels - relatedWidgetWidth);
				QToolTip::showText(posLeft,text);
			} else {
				QToolTip::showText(pos,text);
			}
		}
	}
}


QString truncateLines(const QString & line,int maximum){

	auto lines = line.split('\n');

	const bool exceeds = (lines.count() >= maximum);

	lines = lines.sliced(0,maximum);

	if(exceeds)
		lines.append("...");

	return lines.join('\n');
}


/*
 * Utility function for most recent strings, e.g. for filenames
 * The item is inserted at the front and removed if present in the rest of the list.
 * The list will not get longer than maxLength.
 * Returns true if the list contents changed (i.e. item was not already in first place)
 */

bool addMostRecent(
	const QString & item,
	QStringList & items,
	int length
){
	if(items.contains(item)){

		if(items.first() == item)
			return false;

		items.removeOne(item);
	}

	items.prepend(item);

	if(items.count() > length)
		items.removeLast();

	return true;
}

