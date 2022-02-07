
#include "loghighlighter.h"
#include "Latex/OutputFilter.hpp"


LogHighlighter::LogHighlighter(QTextDocument * document)
	: QSyntaxHighlighter(document) 
	, ColorFile(QColor(0x2b,0x94,0x2b)) {}


inline QRegExp regex(std::string pattern){
	return QRegExp(pattern.c_str());
}

inline std::string all(char c){
	std::string s;
	s = c;
	return "\\" + s + "+";
}


const std::string
	texTypes = "(La|pdf|Lua)TeX",
	space = "\\s*",
	box = "full \\\\[hv]box .*",
	any = ".*";

const auto
	regex_exclamationdots = regex( "!" + all('.') + space ),
	regex_exclamation = regex( all('!') + space ),
	regex_warning = regex( space + "(((! )?" + texTypes + ")|Package) " + any + "Warning" + any + ":" + any ),
	regex_badbox = regex( space + "(Over|Under)" + box ),
	regex_error	= regex( space + "! " + any ),
	regex_stars	= regex( all('*') + space ),
	regex_dots = regex( all('.') + space );


using Entry = LatexLogEntry;


inline QColor LogHighlighter::colorFor(const QString & text) const {

	if(regex_exclamationdots.exactMatch(text))
		return Entry::textColor(LT_ERROR);

	if(regex_exclamation.exactMatch(text))
		return Entry::textColor(LT_ERROR);

	if(regex_error.exactMatch(text))
		return Entry::textColor(LT_ERROR);
	

	if(regex_badbox.exactMatch(text))
		return Entry::textColor(LT_BADBOX);


	if(regex_warning.exactMatch(text))
		return Entry::textColor(LT_WARNING);

	if(regex_stars.exactMatch(text))
		return Entry::textColor(LT_WARNING);


	if(regex_dots.exactMatch(text))
		return Entry::textColor(LT_INFO);
	

	if(!text.contains(".tex"))
		return nullptr;

	if(text.startsWith("Error:"))
		return nullptr;


	return ColorFile;
}


void LogHighlighter::highlightBlock(const QString & text){

	const auto color = colorFor(text);

	if(color == nullptr)
		return;

	setFormat(0,text.length(),color);
}
