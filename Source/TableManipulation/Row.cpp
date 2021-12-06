#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



// Note: Apparently some environments always require a line break "\\". These should be specified here.
// Some don't want one, e.g. align, blockarray, ...
// And some can cope with both cases, e.g. tabular
// Our approach is to remove the last line break unless it's neccesary, either because the
// environment needs it, or there is additional stuff after the line break, such as "\\ \hline".
// See also: https://tex.stackexchange.com/questions/400827/should-i-use-a-line-break-after-the-last-tabular-row


void LatexTables::addRow(QDocumentCursor & c,const int numberOfColumns){
	
	QDocumentCursor cur(c);
	bool stopSearch = false;

	if(cur.columnNumber() > 1){

		cur.movePosition(2,QDocumentCursor::PreviousCharacter,QDocumentCursor::KeepAnchor);
		
		QString res = cur.selectedText();
		
		if(res == "\\\\")
			stopSearch = true;
		
		cur.movePosition(2,QDocumentCursor::NextCharacter);
	}

    const QStringList tokens { "\\\\" , "\\tabularnewline" };

	int result = 0;
	
	if(!stopSearch)
		result = findNextToken(cur,tokens);

    if(result >= 0 || result == -2){

		//if last line before end, check whether the user was too lazy to put in a linebreak
		if(result == -2){

			QDocumentCursor ch(cur);
			
			int res = findNextToken(ch, tokens, true, true);
			
			if(res == -2){
				cur.movePosition(1,QDocumentCursor::PreviousCharacter);
				cur.insertText("\\\\\n");
			} else {

				ch.movePosition(2,QDocumentCursor::NextCharacter,QDocumentCursor::KeepAnchor);
                
				if(ch.selectedText().contains(QRegularExpression("^\\S+$"))){
					cur.movePosition(1,QDocumentCursor::PreviousCharacter);
					cur.insertText("\\\\\n");
				}
			}
		}

		//result=findNextToken(cur,tokens);
		
		cur.beginEditBlock();
		
		if(result > -2)
			cur.insertText("\n");
		
		QString str("& ");
		QString outStr(" ");

		for(int i = 1;i < numberOfColumns;i++)
			outStr += str;
		
		cur.insertText(outStr);
		cur.insertText("\\\\");

		if(!cur.atLineEnd())
			cur.insertText("\n");
		
		cur.endEditBlock();
	}
}


void LatexTables::removeRow(QDocumentCursor & c){

	QDocumentCursor cur(c);
    
	const QStringList tokens { "\\\\" , "\\tabularnewline" };
	
	if(cur.hasSelection())
		if(
			cur.lineNumber() > cur.anchorLineNumber() || 
			(cur.lineNumber() == cur.anchorLineNumber() && cur.columnNumber() > cur.anchorColumnNumber())
		) cur.moveTo(cur.anchorLineNumber(), cur.anchorColumnNumber());
	
	int result = findNextToken(cur, tokens, false, true);
    
	if(result >= 0)
        cur.movePosition(tokens[result].length(),QDocumentCursor::NextCharacter);
	
	if(result == -2)
		cur.movePosition(1,QDocumentCursor::EndOfLine);
	
	bool outOfTokens = false;
	
	do {
		outOfTokens = (findNextToken(cur,tokens,true) == -1);
		
		if(outOfTokens)
			break;

	} while(c.isWithinSelection(cur));

	// while(!(breakLoop = (findNextToken(cur, tokens, true) == -1)) && c.isWithinSelection(cur)){}
	
	if(outOfTokens)
		return;

	// check if end of cursor is at line end
	
	QDocumentCursor c2(cur.document(),cur.anchorLineNumber(),cur.anchorColumnNumber());
	
	if(c2.atLineEnd()){
		c2.movePosition(1,QDocumentCursor::NextCharacter);
		cur.moveTo(c2,QDocumentCursor::KeepAnchor);
	}
	
	// remove text
	
	cur.beginEditBlock();
	cur.removeSelectedText();

	if(cur.line().text().isEmpty())
		cur.deleteChar(); // don't leave empty lines

	cur.endEditBlock();
}
