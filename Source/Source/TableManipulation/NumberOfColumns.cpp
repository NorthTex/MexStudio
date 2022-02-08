#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



// get the number of columns which are defined by the the tabular (or alike) env

int LatexTables::getNumberOfColumns(QDocumentCursor & cur){

	QDocumentCursor c(cur);

	int result = findNextToken(c,QStringList(),false,true);
	
	if(result != -2)
		return -1;
	
	QString line = c.line().text();
	
	int pos = line.indexOf("\\begin");
	
	if(pos > -1){
		QStringList values;
		LatexParser::resolveCommandOptions(line,pos,values);
		return getNumberOfColumns(values);
	}

	return -1;
}


// get the number of columns which are defined by the the tabular (or alike) env, strings contain definition

int LatexTables::getNumberOfColumns(QStringList values){

	if(values.isEmpty())
		return -1;
	
	QString env = values.takeFirst();
	
	if(!env.startsWith("{"))
		return -1;
		
	if(!env.endsWith("}"))
		return -1;
	
	env = env.mid(1);
	env.chop(1);
	
	int numberOfOptions = -1;
	
	if(tabularNames.contains(env))
		numberOfOptions = 0;
	
	if(tabularNamesWithOneOption.contains(env))
		numberOfOptions = 1;
	
	if(numberOfOptions < 0)
		return -1;

	while(!values.isEmpty()){
		
		QString opt = values.takeFirst();
		
		if(opt.startsWith("[") && opt.endsWith("]"))
			continue;
		
		if(numberOfOptions > 0){
			numberOfOptions--;
			continue;
		}

		if(!opt.startsWith("{"))
			return -1;
			
		if(!opt.endsWith("}"))
			return -1;
		
		opt = opt.mid(1);
		opt.chop(1);
		
		//calculate number of columns ...
		QStringList res = splitColDef(opt);
		
		int cols = res.count();
		
		//return result
		return cols;
	}

	return -1;
}

// return number of columns a \multicolumn command spans (number in first braces)
// optionally string pointers may be passed to obtain alignment and the text

LatexTables::Span LatexTables::getNumOfColsInMultiColumn(const QString & str){

	//return the number of columns in mulitcolumn command

	QStringList values;
	QString zw = str.trimmed();

	LatexParser::resolveCommandOptions(zw,0,values);

	if(values.length() != 3)
		return {};

	QString alignment , text;

	alignment = LatexParser::removeOptionBrackets(values.at(1));
	text = LatexParser::removeOptionBrackets(values.at(2));;

	zw = values.takeFirst();
	
	if(zw.startsWith("{") && zw.endsWith("}")){

		zw.chop(1);
		zw = zw.mid(1);
		
		bool ok;
		
		int col = zw.toInt(& ok);
		
		if(ok)
			return { col , alignment , text };
	}
	
	return {};
}
