#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



void LatexTableLine::setColLine(const QString line){

	colLine = line;
	
	int start = 0;

	for(int i = 0;i < line.length();i++){

		if(i > 0 && line.at(i - 1) == '\\')
			continue;

		if(line.at(i) != '&')
			continue;

		appendCol(line.mid(start,i - start));

		if(i < line.length())
			start = i + 1;
	}
	
	if(start <= line.length())
		appendCol(line.mid(start));
}


QString LatexTableLine::colText(int column,int width,const QChar & alignment){

	const auto value = cols.at(column);
	const auto space = width - value.length();

	switch(alignment.toLower().toLatin1()){
	case 'r': // Left
		return QString(space,' ') + value;
	case 'c': // Center
		return QString(space * 0.5,' ') + value + QString((space + 1) * 0.5,' ');
	case 'l': // Right
		return value + QString(space,' ');
	default:
		return value;
	}
}


const QSet<QString> alignments { 
	"l" , "left" ,
	"c" , "center" , 
	"r" , "right" 
};


void LatexTableLine::appendCol(const QString & column){

	auto append = [ & ](const QString value,const MultiColFlag flag,const QChar alignment){
		cols.append(value.trimmed());
		mcFlag.append(flag);
		mcAlign.append(alignment);
	};


	if(column.contains("\\multicolumn")){

		auto [ columnsInSpan , align , _ ] = LatexTables::getNumOfColsInMultiColumn(column);

		if(!alignments.contains(align))
			align = 'l';

		const auto alignment = align.at(0);

		if(columnsInSpan > 1){

			append(column,MCStart,alignment);

			columnsInSpan -= 2;

			// inserting dummy rows

			while(columnsInSpan--)
				append("",MCMid,alignment);

			append("",MCEnd,alignment);
		} else {
			
			// not really a span

			append(column,MCNone,alignment);
		}
		
	} else {
		append(column,MCNone,QChar());
	}
}
