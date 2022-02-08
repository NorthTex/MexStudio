#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"



void remove(QDocumentCursor,const int,QStringList *);

void LatexTables::removeColumn(QDocument * doc,const int line,const int column,QStringList * cutBuffer){

	QDocumentCursor cursor(doc);

	//preparations for search
	
	cursor.moveTo(line,0);

	remove(cursor,column,cutBuffer);
}


const QStringList tokens { 
	"\\\\" , "\\tabularnewline" , "\\&" , "&" };

const QVector<QString> elementsToKeep { 
	"\\hline" , "\\endhead" , "\\endfoot" , "\\endfirsthead" , "\\endlastfoot" };



void remove(QDocumentCursor cursor,const int column,QStringList * cutBuffer){
	
	QString def = LatexTables::getDef(cursor);
	
	if(def.isEmpty())
		return; // begin not found

	cursor.beginEditBlock();

	//remove col in definition
	QStringList defs = LatexTables::splitColDef(def);
	
	def.clear();
	
	for(int i = 0;i < defs.count();i++)
		if(i == column){
			if(cutBuffer)
				cutBuffer -> append(defs[i]);
		} else {
			def.append(defs[i]);
		}

	if(def.isEmpty())
		cursor.removeSelectedText();
	else
		cursor.insertText(def);

	cursor.movePosition(2,QDocumentCursor::NextCharacter);
	
	// remove column
	QString line;
	bool breakLoop = false;

	while(!breakLoop){

        int off = 0 , result = 3;
		
		for(int col = 0;col < column;col++){

			if(off > 0)
				off--;
			
			cursor.clearSelection();
			
			QDocumentCursor oldCur(cursor);
			
			do { result = LatexTables::findNextToken(cursor,tokens,true); }
			while (result == 2);
			
			QString selected = cursor.selectedText();

			if(selected.startsWith("\\multicolumn")){
				
				auto columnCount = LatexTables::getNumOfColsInMultiColumn(selected).columns;

				if(off == 0)
					off = columnCount;
				
				if(off > 1)
					cursor = oldCur;
			}

			breakLoop = (result < 0); // end of tabular reached (eof or \end)
			
			if(result < 1)
				break; //end of tabular line reached
		}

		cursor.clearSelection();
		
		if(result == -1)
			break;
		
		// add element
        if(result > 1 || off){

			do { result = LatexTables::findNextToken(cursor,tokens,true); }
			while (result == 2);
			
			QString selText = cursor.selectedText();
			int add = 0;
			
			if(selText.startsWith("\\multicolumn"))
				add = LatexTables::getNumOfColsInMultiColumn(selText).columns;


            auto moveToPreviousAt = [ & ](int index){
                cursor.movePosition(index,QDocumentCursor::PreviousCharacter,QDocumentCursor::KeepAnchor);
            };

			if(add > 1){

				//multicolumn handling
				QStringList values;
				
				LatexParser::resolveCommandOptions(selText,0,values);
				
				values.takeFirst();
				values.prepend(QString("{%1}").arg(add - 1));

                moveToPreviousAt(1);
                
				if(result == 0)
					moveToPreviousAt(1);
                
				if(result == 1)
                    moveToPreviousAt(14);

				cursor.insertText("\\multicolumn" + values.join(""));
			} else {

				//normal handling
                if(result == 3 && column > 0)
                    moveToPreviousAt(1);

				if(result == 0)
                    moveToPreviousAt(2);

                if (result == 1)
                    moveToPreviousAt(15);

                QString zw = cursor.selectedText();

				if(cutBuffer){
                    
					if(column == 0 && result > 1)
						zw.chop(1);
					
					cutBuffer -> append(zw);
				}

				// detect elements which need to be kept like \hline
				QString keep;
				
				if(column == 0){


					for(int i = 0; i < zw.length();i++){

						if(zw.at(i) == '\n')
							if (!keep.endsWith('\n'))
								keep += "\n";

						//commands
						if(zw.at(i) == '\\'){

							QRegExp rx("\\w+");
							rx.indexIn(zw,i + 1);
							
							QString cmd = "\\" + rx.cap();

							if(elementsToKeep.contains(cmd))
								keep += " " + cmd;
						}
					}

                    if(keep.length() == 1)
						keep.clear();
				}

				cursor.removeSelectedText();
				
				if(column > 0){
					cursor.movePosition(1,QDocumentCursor::PreviousCharacter,QDocumentCursor::KeepAnchor);
					cursor.removeSelectedText();
				}

				cursor.insertText(keep);
			}

            const QStringList tokens { "\\\\" , "\\tabularnewline" };
			
			breakLoop = (LatexTables::findNextToken(cursor,tokens) == -1);
		}

		if(cursor.atLineEnd())
			cursor.movePosition(1,QDocumentCursor::NextCharacter);
		
		line = cursor.line().text();
		
		if(line.contains("\\end{"))
			breakLoop = true;
	}

	cursor.endEditBlock();
}
