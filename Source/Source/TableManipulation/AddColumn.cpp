#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



void LatexTables::addColumn(QDocument * doc,const int lineNumber,const int afterColumn,QStringList * cutBuffer){
	
	QDocumentCursor cur(doc);
	
	QStringList pasteBuffer , nTokens;

    nTokens << "\\\\" << "\\tabularnewline" << "\\&" << "&";
	
	if(cutBuffer){
	
		pasteBuffer = * cutBuffer;
	
		if(pasteBuffer.size() == 0)
			return;
	}

	cur.beginEditBlock();
	cur.moveTo(lineNumber,0);

	QString def = getDef(cur);
	
	//int result=findNextToken(cur,QStringList(),false,true); // move to \begin{...}
	
	if(def.isEmpty()) {
		cur.endEditBlock();
		return; // begin not found
	}

	//add col in definition
	QStringList defs = splitColDef(def);
	QString addCol = "l";
	
	if(cutBuffer)
		addCol = pasteBuffer.takeFirst();

	def.clear();

	if(afterColumn == 0)
		def = addCol;
	
	for(int i = 0; i < defs.count();i++){

		def.append(defs[i]);
		
		if(i + 1 == afterColumn || (i + 1 == defs.count() && i + 1 < afterColumn))
			def.append(addCol);
	}

	cur.insertText(def);
	//continue adding col
	cur.movePosition(2, QDocumentCursor::NextCharacter);

	QString line;
	bool breakLoop = false;
    int result = 3;

	while(!breakLoop){

		for(int col = 0;col < afterColumn;col++){

			do {
				result = findNextToken(cur,nTokens);
            } while (result == 2);
            
			if(result < 2)
				break; //end of tabular line reached
		}

		if(result == -1)
			break;
		
		//if last line before end, check whether the user was too lazy to put in a linebreak
		
		if(result == -2){

			QDocumentCursor ch(cur);
            QStringList tokens { "\\\\" , "\\tabularnewline" };
			
			int res = findNextToken(ch,tokens,true,true);

			if(res == 0){
			
				ch.movePosition(2, QDocumentCursor::NextCharacter, QDocumentCursor::KeepAnchor);
            
			    if(ch.selectedText().contains(QRegularExpression("^\\S+$")))
					break;
			}
		}

		// add element
        if(result == 3){
			if(pasteBuffer.isEmpty()){
				cur.insertText(" &");
			} else {
				if(afterColumn == 0){
					
					QDocumentCursor ch(cur);
					int res = findNextToken(ch,nTokens);
					
					cur.insertText(pasteBuffer.takeFirst());
		
					if(res != 0)
						cur.insertText("&");
				} else {
					cur.insertText(pasteBuffer.takeFirst() + "&");
				}

			}
		}

        if(result <= 1){

            int count =  1;
          
		    switch(result){
            case 0: 
				count = 2;
                break;
            case 1: 
				count = 15;
                break;
            }
			
			cur.movePosition(count,QDocumentCursor::PreviousCharacter);

			if(pasteBuffer.isEmpty())
				cur.insertText("& ");
			else
				cur.insertText("&" + pasteBuffer.takeFirst());
		}

        const QStringList tokens { "\\\\" , "\\tabularnewline" };

		breakLoop = (findNextToken(cur,tokens) == -1);

		// go over \hline if present
		QString text = cur.line().text();
		int col = cur.columnNumber();
		
		text = text.mid(col);
		
		QRegExp rxHL("^(\\s*\\\\hline\\s*)");
		int pos_hline = rxHL.indexIn(text);
		
		if(pos_hline > -1){
			int l = rxHL.cap().length();
			cur.movePosition(l, QDocumentCursor::NextCharacter);
		}

		if(cur.atLineEnd())
			cur.movePosition(1, QDocumentCursor::NextCharacter);
		
		line = cur.line().text();
		
		if(line.contains("\\end{"))
			breakLoop = true;
	}

	cur.endEditBlock();
}