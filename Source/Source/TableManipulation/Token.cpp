#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"


// find next element starting from cursor whhich is in the string list "tokens". The element which is closest to the cursor is returned
// return >=0 : position of stringlist which was detected
// return -1 : end of file was found, no element detected
// return -2 : \end{...} was detected (the env-name is not tested)
//
// the cursor is moved right behind the detected token ! (or in front in case of backward search)

int LatexTables::findNextToken(QDocumentCursor & cur,QStringList tokens,bool keepAnchor,bool backwards){
	
	int pos = -1;
	int nextToken = -1;
	int offset = 0;
	

	QDocumentCursor::MoveOperation mvNextLine = backwards ? QDocumentCursor::PreviousLine : QDocumentCursor::NextLine;
	QDocumentCursor::MoveOperation mvNextChar = backwards ? QDocumentCursor::PreviousCharacter : QDocumentCursor::NextCharacter;
	QDocumentCursor::MoveOperation mvStartOfLine = backwards ? QDocumentCursor::EndOfLine : QDocumentCursor::StartOfLine;
	QDocumentCursor::MoveFlag mvFlag = keepAnchor ? QDocumentCursor::KeepAnchor : QDocumentCursor::MoveAnchor;
	
	do {
		
		QString line = cur.line().text();

		if(backwards)
			offset = line.length();

		line = LatexParser::cutComment(line);
		
		if(backwards){
			offset = offset - line.length();
            std::reverse(line.begin(),line.end());
		}

		if(line.contains("\\end{") && !backwards){
			nextToken = -2;
			break;
		}

		if(line.contains("{nigeb\\") && backwards){
			nextToken = -2;
			break;
		}

		pos = -1;
		
		for(int i = 0;i < tokens.count();i++){

			QString elem = tokens.at(i);
            
            // reverse element as it was done with line

			if(backwards)
                std::reverse(elem.begin(),elem.end());

			int colNumber = cur.columnNumber();

			if(backwards) 
				colNumber = line.length() + offset - colNumber ;
			
			int zw = line.indexOf(elem,colNumber);
			
			if(zw > -1)
				if(pos > zw || pos == -1){
					pos = zw;
					nextToken = i;
				}
		}

		if(pos < 0){

			if(!backwards && cur.lineNumber() >= cur.document()->lineCount() - 1)
				break;

			if(backwards && cur.lineNumber() <= 0)
				break;
			
			cur.movePosition(1,mvNextLine,mvFlag);
			cur.movePosition(1,mvStartOfLine,mvFlag);
		}

	} while (pos < 0);
	
	if(pos > -1){
		cur.movePosition(1, mvStartOfLine, mvFlag);
		cur.movePosition(pos + tokens.at(nextToken).length() + offset, mvNextChar, mvFlag);
	}

	return nextToken;
}