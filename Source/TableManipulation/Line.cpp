#include "tablemanipulation.h"
#include "qdocumentcursor.h"
#include "qdocumentline.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "scriptengine.h"
#include "configmanager.h"




// add \hline and end of rows (remove==true => remove instead)
// start from cursor position for numberOfLines ( until end if -1 )

void LatexTables::addHLine(QDocumentCursor & cur,const int numberOfLines,const bool remove){
	
	QDocumentCursor c(cur);
	c.beginEditBlock();
    
	QStringList tokens { "\\\\" , "\\tabularnewline" };
	QStringList hline("\\hline");
	
	int ln = numberOfLines;

	while(ln != 0){

		int result = findNextToken(c,tokens);
		
		if(result < 0)
			break;
		
		if(remove){

			QDocumentCursor c2(c);
			result = findNextToken(c,hline,true);
            
			if(c.selectedText().contains(QRegularExpression("^\\s*\\\\hline$")))
				c.removeSelectedText();
			else
				c = c2;
		} else {

			// don't add \hline if already present
			QString text = c.line().text();
			
			int col = c.columnNumber();
			int pos_hline = text.indexOf(" \\hline",col);
            
			if(pos_hline < 0 || !text.mid(col, pos_hline - col).contains(QRegularExpression("^\\s*$"))){

				c.insertText(" \\hline");
				
				if(!c.atLineEnd())
					c.insertText("\n");
			}
		}

		ln--;
	}
	
	c.endEditBlock();
}



// parses text up to the end of the next line. Returns column after the line in startCol
// return value is 0 in case of error

LatexTableLine * LatexTableModel::parseNextLine(const QString & text,int & startCol){

	QString pre;
	QString line;
    QString lineBreakOption{"\\\\"};

	int endCol = findRowBreak(text,startCol);

	if(endCol < 0){
		line = text.mid(startCol).trimmed();
        //endCol = text.length();
	} else {
	
		line = text.mid(startCol, endCol - startCol).trimmed();
    
	    if(text.mid(endCol,2)=="\\\\"){
            endCol += 2; // now behind "\\"
        } else {
            endCol += 15;
            lineBreakOption = "\\tabularnewline";
        }

        // check for line break with * (nopagebreak, i.e. \\*)
        if(endCol < text.length() - 1 && text[endCol] == '*'){
            endCol++;
            lineBreakOption += "*";
        }

		// check for line break with option, e.g. \\[1em]
		if(endCol < text.length() - 1 && text[endCol] == '['){

			int startOpt = endCol;
			int endOpt = findClosingBracket(text, endCol, '[', ']');
			
			if(endOpt < 0){
				UtilsUi::txsWarning("Could not parse table code: Missing closing bracket: \\[");
                return nullptr;
			}

            lineBreakOption += text.mid(startOpt,endOpt - startOpt + 1).trimmed();
			endCol = endOpt + 1;
		}
	}

    if(lineBreakOption == "\\\\")
        lineBreakOption.clear(); // default is used

	// ceck for meta line commands at beginning of line
	bool recheck = true;

    if(line.startsWith("%") && recheck){

        int behind = line.indexOf(QRegularExpression("[\\n\\r]"));
        pre.append(line.left(behind));

        line = line.mid(behind).trimmed();
        recheck = true;
    }

    while(line.startsWith("\\") && recheck){

		recheck = false;
		
		foreach(const QString & cmd,metaLineCommands){
			if(line.startsWith(cmd)){

				int behind;
				
				getCommandOptions(line,cmd.length(),& behind);
                
				if(behind > 0 && pre.contains('%'))
                    pre.append('\n');

				pre.append(line.left(behind));

				line = line.mid(behind).trimmed();
				recheck = true;

				break;
			}
		}
	}

	auto ltl = new LatexTableLine(this);
	
	if(!pre.isEmpty())
		ltl -> setMetaLine(pre);

    if(!line.isEmpty() || endCol > startCol)
		ltl -> setColLine(line); // allow empty columns
	
	if(!lineBreakOption.isEmpty())
		ltl -> setLineBreakOption(lineBreakOption);

    startCol = endCol;

    if(startCol<0)
        startCol = text.length();

	return ltl;
}



QStringList LatexTableModel::getAlignedLines(const QStringList alignment,const QString & rowIndent,bool forceLineBreakAtEnd) const {

	QString delim = " & ";
	QVector<QString> cl(lines.count());
	QVector<int> multiColStarts(lines.count());
	
	for(int i = 0;i < lines.count();i++)
		multiColStarts[i] = -1;
	
	QStringList alignTokens(alignment);

	foreach(LatexTableLine * tl,lines){
		
		// fallback to 'l' if more columns are there than specified in alignment
		while (alignTokens.length() < tl -> colCount())
			alignTokens.append("l");
	}

	bool oneLinePerCell = ConfigManager::getInstance() -> getOption("TableAutoformat/One Line Per Cell").toBool();
	
	QString colSep = " & ";

	if(oneLinePerCell) 
		colSep = " &\n" + rowIndent;

	int pos = 0;

	for(int col = 0;col < alignTokens.length();col++){
		
		// col width detection
		int width = 0;

		for(int row = 0;row < lines.length();row++){
			
			auto tl = lines.at(row);
			
			if(col >= tl -> colCount())
				continue;
			
			switch(tl -> multiColState(col)){
			case LatexTableLine::MCStart:
				multiColStarts[row] = pos;
				break;
			case LatexTableLine::MCMid:
				break;
			case LatexTableLine::MCEnd: {

				int startCol = tl -> multiColStart(col);
			
				Q_ASSERT(startCol >= 0);
				
				int w = tl -> colWidth(startCol) - (pos - multiColStarts[row]);
				
				if(width < w)
					width = w;
			}
			break;
			default:
				int w = tl -> colWidth(col);
				if (width < w) width = w;
			}
		}

		// size and append cols

		QChar align = alignTokens.at(col).at(0);
		
		for(int row = 0;row < lines.length();row++){

			auto tl = lines.at(row);
			
			if(col < tl -> colCount()){
				if(tl -> multiColState(col) == LatexTableLine::MCEnd){

					int startCol = tl -> multiColStart(col);
					
					Q_ASSERT(startCol >= 0);
					
					cl[row].append(tl -> colText(startCol,width + (pos - multiColStarts[row]),tl -> multiColAlign(startCol)));
					
					if(col < alignTokens.length() - 1)
						cl[row].append(colSep);

					multiColStarts[row] = -1;
				} else 
				if(tl -> multiColState(col) == LatexTableLine::MCNone){

					cl[row].append(tl -> colText(col,width,align));
					
					if(col < alignTokens.length() - 1)
						cl[row].append(colSep);
				}
			}
		}

		pos += width + delim.length();
	}

	QStringList ret;

	for(int row = 0;row < lines.count();row++){
		
		QString ml = lines.at(row) -> toMetaLine();
        QString lbo = lines.at(row) -> toLineBreakOption();
        QString lineTerm = lbo.isEmpty() ? " \\\\" : " "+lbo;
		
		switch(metaLineCommandPos){
		// keep in sync with options configdialog.ui
		case 0: // Behind line break
			
			if(!ml.isEmpty())
				if(row == 0)
					ret.append(rowIndent + ml);
				else
					ret.last().append(" " + ml);
			
			if(!cl[row].isEmpty())
				ret.append(rowIndent + cl[row] + lineTerm);
			
			break;
		case 1: // separate line (no indent)

			if(!ml.isEmpty())
				ret.append(ml);
			
			if(!cl[row].isEmpty())
				ret.append(rowIndent + cl[row] + lineTerm);
			
			break;
		case 2: // separate line (indent to first col)
			
			if(!ml.isEmpty())
				ret.append(rowIndent + ml);
			
			if(!cl[row].isEmpty())
				ret.append(rowIndent + cl[row] + lineTerm);
			
			break;
		default:
            qDebug("Invalid metaLineCommand pos");
		}

	}

	if(!ret.isEmpty() && !forceLineBreakAtEnd){

		// smart removal of break "\\" at final line:
		// except for cases like "\\ \hline" and empty column
		QString & last = ret.last();
        
		if(last.endsWith("\\\\") || last.endsWith("\\tabularnewline")){

            QString zw = trimRight(last.left(last.length() - 2));
            
			if(!zw.isEmpty())
                last = zw;
		}
	}

	return ret;
}